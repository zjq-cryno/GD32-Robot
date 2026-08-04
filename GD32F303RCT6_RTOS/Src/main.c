
/***
 *                    _ooOoo_
 *                   o8888888o
 *                   88" . "88
 *                   (| -_- |)
 *                    O\ = /O
 *                ____/`---'\____
 *              .   ' \\| |// `.
 *               / \\||| : |||// \
 *             / _||||| -:- |||||- \
 *               | | \\\ - /// | |
 *             | \_| ''\---/'' | |
 *              \ .-\__ `-` ___/-. /
 *           ___`. .' /--.--\ `. . __
 *        ."" '< `.___\_<|>_/___.' >'"".
 *       | | : `- \`.;`\ _ /`;.`/ - ` : | |
 *         \ \ `-. \_ __\ /__ _/ .-` / /
 * ======`-.____`-.___\_____/___.-`____.-'======
 *                    `=---='
 *
 * .............................................
 *          佛祖保佑             永无BUG
 */

#include "includes.h"
#include "app.h"
#include "bmi270.h"
#include "gd32_adapter.h"
#include "cmsis_os.h"

uint8_t uart1_ch; // 用于UART1接收中断的临时变量
uint8_t uart5_ch; // 用于UART5接收中断的临时变量

uint8_t uart1_rxbuf[UART1_RX_BUF_SIZE] = {0}; // 串口1接收缓存
uint8_t uart1_rx_cnt = 0;                     // 串口1接收字节计数
uint8_t uart1_rx_flag = 0;                    // 串口1接收完成标志（0：未完成，1：完成）
uint8_t uart5_txbuf[UART5_TX_BUF_SIZE] = {0}; // 串口5发送缓存
uint8_t uart5_rxbuf[UART5_TX_BUF_SIZE] = {0}; // 串口5接收缓存（若未使用可保留初始化）

uint8_t recv_end_flag_huart5 = 0;

// 全局变量
float adc_value;   //adc采集数据
int i;
//bmi
int bmi_len,adc_len;           // 存储实际发送的长度
int8_t bmi_id, CAN_ack=0;
uint16_t hg25q256_id = 0;

uint16_t time_tim7,time_out,time;
uint32_t tick;

uint8_t rx_frame_index = 0;
uint8_t rx_frame[32]={0}; // 存储完整帧


// 用于缓存一帧
uint8_t ota_frame_buf[8];

osMutexId uart5MutexHandle;
osMutexId g_mutex_printf;
osMutexId logMutexHandle;

//osMessageQId canRxQueueHandle;
QueueHandle_t canRxQueueHandle;

void SystemClock_Config(void);
void MX_FREERTOS_Init(void);

void My_UART_Transmit(UART_HandleTypeDef *huart, uint8_t tx_frame[], int len)
{
		HAL_StatusTypeDef ret;

		osMutexWait(uart5MutexHandle, osWaitForever); // 加锁
		ret = HAL_UART_Transmit(&huart5, tx_frame, len, HAL_MAX_DELAY);
		if(ret == HAL_OK){
			osMutexRelease(uart5MutexHandle); // 解锁
			uart1_print("HAL_UART_Transmit HAL_OK\r\n");
		}else if(ret == HAL_ERROR){
			uart1_print("HAL_UART_Transmit HAL_ERROR\r\n");
		}else if(ret == HAL_BUSY){
			uart1_print("HAL_UART_Transmit HAL_BUSY\r\n");
		}else if(ret == HAL_TIMEOUT){
			uart1_print("HAL_UART_Transmit HAL_TIMEOUT\r\n");
		}
}	


void uart1_print(const char *fmt, ...) {
    char buf[128];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if(len > 0)
    {
        osMutexWait(g_mutex_printf, osWaitForever);
        HAL_UART_Transmit(&huart1, (uint8_t*)buf, len, HAL_MAX_DELAY);
        osMutexRelease(g_mutex_printf);  // 解锁
    }
}
void Log(uint8_t log_level,const char *fmt, ...) {
    char buf[128];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if(len > 0)
    {  
				osMutexWait(logMutexHandle, osWaitForever); 
				uint8_t write_ret = Log_WriteToFlash_DMA(log_level, buf);
				if (write_ret  == 0) {
					printf("Log写入成功，addr:0x%X, 内容：%s\r\n", g_log_current_addr, buf);
					Log_UpdateWriteAddr();  //更新地址  
					
        } else if(write_ret  == 1){
					printf("参数错误\r\n");
          
        }else if(write_ret  == 2){
					printf("DMA写入失败\r\n");
				}else if(write_ret  == 3){
					printf("校验读取失败\r\n");
				}
				osMutexRelease(logMutexHandle);  // 解锁
    }
}
void LED_Blink(void)
{
    // LED亮
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,GPIO_PIN_SET);
    HAL_Delay(100);
    
    // LED灭
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,GPIO_PIN_RESET);
   HAL_Delay(100);
}
int main(void)
{
 
	//重设向量表
	SCB->VTOR = 0x08004000;
	
  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();
  MX_UART5_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();  //交互板通信/UART/CAN
	MX_USART1_UART_Init();  //串口打印
	MX_DMA_Init();      //DMA初始化不在SPI前 会出现失效的情况
	MX_ADC1_Init();
	MX_I2C2_Init();
	MX_TIM6_Init();
	MX_TIM7_Init();
	MX_SPI2_Init();
  MX_SPI1_Init();
	MX_BMI270_Init();
	MX_CAN_Init();
	
	HAL_UART_Receive_IT(&huart1, &uart1_ch, 1); // 启动UART1接收中断
  HAL_UART_Receive_IT(&huart5, &uart5_ch, 1); // 启动UART5接收中断
	
	
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_8,GPIO_PIN_SET);  //舵机供电使能
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_3,GPIO_PIN_SET); //adc使能
	
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_3,GPIO_PIN_SET); //开关机功能使能
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_6,GPIO_PIN_SET); //交互板供电使能
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,GPIO_PIN_SET);
 
	if (Log_StorageInit() != 0) {
			// 初始化失败处理
			printf("Log存储初始化失败\r\n");
	}
	
	bmi_id = bmi270_read_id(&bmi);
	if(bmi_id == -1){
		printf("bmi270_read_id fail\r\n");
	}else{
		printf("bmi_id=%d\r\n",bmi_id);
	}
	
	hg25q256_id = hg25q256mw_read_id();
	if(hg25q256_id == 0){
		printf("hg25q256mw_read_id fail\r\n");
	}else{
		printf("hg25q256_id=%d\r\n",hg25q256_id);
	}

	
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,GPIO_PIN_SET);
	HAL_Delay(100);
	SPI1_RST_0;
	HAL_Delay(1000);
	SPI1_RST_1;
	HAL_Delay(1000);
	TFT_init();
	TFT_full(RED);
	//TFT_clear();	
	//HAL_Delay(1000);
	
	HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
	
	HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);    
	HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);     
	HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 6, 0);    
	HAL_NVIC_SetPriority(UART5_IRQn, 6, 0);      
	HAL_NVIC_SetPriority(USART1_IRQn, 6, 0);
	
	//HG25Q256_Write_Multiple_Pages(IMAGE2_FLASH_ADDR, gImage_5, TOTAL_BYTES);
	//DisplayAnimationFromFlash();
	
	//init_all_servos(&huart5);  //开机上电读取舵机当前位置作为零点
	
	
	
	MX_FREERTOS_Init();
  osKernelStart();
	
  //FWDGT_Init();
  while (1){
		
		// LED闪烁指示程序运行
    //LED_Blink();
		//HAL_Delay(1000);
		// 定期喂狗，确保间隔小于3.2秒
    //FWDGT_Feed();
	}
  

}


void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

 
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
	 PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}


// 发送完成回调，清除忙标志
void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
   if (huart->Instance == UART5)
	{
    printf("UART_TxCallback\r\n");
	}
}

// UART接收中断回调函数
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) 
{
		if(huart->Instance == USART1)  // 检查是UART1的中断
    {
        // 将接收到的字节存入缓冲区
        uart1_rxbuf[uart1_rx_cnt++] = uart1_ch;
        
        // 判断接收终止条件
        if(uart1_ch == END_CHAR || uart1_rx_cnt >= UART1_RX_BUF_SIZE - 1)
        {
            uart1_rxbuf[uart1_rx_cnt] = '\0'; // 添加字符串结束符
            uart1_rx_flag = 1; // 设置接收完成标志
            uart1_rx_cnt = 0;  // 重置计数器（可选，在主循环中重置也可以）
        }
        else
        {
            // 重新启动中断接收
            HAL_UART_Receive_IT(&huart1, &uart1_ch, 1);
        }
    }
		
		if (huart->Instance == UART5)
		{
			// 1. 检查帧头：舵机应答帧必须以0xAC开头
			if (rx_frame_index == 0) {
					if (uart5_ch != 0xAC) {
							rx_frame_index = 0; // 丢弃无效数据
							HAL_UART_Receive_IT(&huart5, &uart5_ch, 1);// 帧头错误，重新接收下一字节
							return;
					}
			}
			// 2. 存储当前字节
			rx_frame[rx_frame_index++] = uart5_ch;

			// 3. 检查是否收到完整帧（最小长度5字节）
			if (rx_frame_index >= 4) {
					
					uint8_t frame_length = rx_frame[2]; // 从帧中提取长度字段
					uint8_t servo_id = rx_frame[1];     // 从帧中提取舵机id
					// 3.1 检查是否已接收完整帧
					if (rx_frame_index >= frame_length-1) {
							
							// 3.2 校验和验证
							uint8_t calculated_checksum = calculate_checksum(rx_frame, frame_length);
							if (calculated_checksum == rx_frame[frame_length - 1]) {
									//memcpy(uart5_rxbuf, rx_frame, strlen((char *)rx_frame));
									//uint8_t len = strlen(uart5_rxbuf);
									//HAL_UART_Transmit(&huart1, uart5_txbuf, len, HAL_MAX_DELAY);
									//printf("%\r\n",uart5_rxbuf);
									// 校验通过，解析应答帧
									parse_servo_response(rx_frame, frame_length);
									
									update_servo[servo_id-1] = true;		
							} else {
									printf("Checksum error\r\n");
							}
							rx_frame_index = 0; // 重置索引
									
					}
			}

			// 4. 继续接收下一字节
			HAL_UART_Receive_IT(&huart5, &uart5_ch, 1);
		}		
}
// 字符串转十六进制函数
void string_to_hex(uint8_t *dest, char *src, int *length) {
    char temp[3] = {0};
    int i = 0;
    *length = 0;
    
    while(src[i] != '\0' && src[i+1] != '\0') {
        if(src[i] != ' ') { // 跳过空格
            temp[0] = src[i];
            temp[1] = src[i+1];
            dest[(*length)++] = (uint8_t)strtol(temp, NULL, 16);
            i += 2;
        } else {
            i++; // 跳过空格
        }
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  
  if (htim->Instance == TIM4) {
    HAL_IncTick();
  }
	
}


// DMA接收完成回调函数
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI2) {
        // DMA传输完成
      dma_transfer_complete = 1;  
			HG25Q256_Disable();
    }
		
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1) {
			SPI1_CS_1;// 结束传输
			dma_transfer_complete_spi1 = 1;  //设置标志位
    }
		if (hspi->Instance == SPI2) {
			dma_transfer_complete = 1;;  
    }
}


void Error_Handler(void)
{
  
  __disable_irq();
  while (1)
  {
  }
 
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */


// 开始计时
void start_time(uint32_t *start_tick) {
	*start_tick = xTaskGetTickCount();
}



// 停止计时并返回经过的时间（毫秒）
uint32_t stop_time(uint32_t start_tick) {
    uint32_t end_tick = xTaskGetTickCount();
    return (end_tick - start_tick) * portTICK_PERIOD_MS;
}



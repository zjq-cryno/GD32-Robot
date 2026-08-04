#include "includes.h"
#include "app.h"
#include "bmi270.h"
#include "gd32_adapter.h"

// 接收缓冲区
uint8_t rxBuffer5[10]; 
uint16_t rxIndex5 = 0; 
uint8_t rxBuffer3[10]; 
uint16_t rxIndex3 = 0; 
uint8_t rxBuffer1[10]; 
uint16_t rxIndex1 = 0; 



uint8_t uart1_rxbuf[UART1_RX_BUF_SIZE] = {0}; // 串口1接收缓存
uint8_t uart1_rx_cnt = 0;                     // 串口1接收字节计数
uint8_t uart1_rx_flag = 0;                    // 串口1接收完成标志（0：未完成，1：完成）
uint8_t uart5_txbuf[UART5_TX_BUF_SIZE] = {0}; // 串口5发送缓存
uint8_t uart5_rxbuf[UART5_TX_BUF_SIZE] = {0}; // 串口5接收缓存（若未使用可保留初始化）

uint8_t recv_end_flag_huart5 = 0;

//变量区域
// 定义临时变量用于中断接收
uint8_t uart1_ch; // 用于UART1接收中断的临时变量
uint8_t uart5_ch; // 用于UART5接收中断的临时变量
uint32_t uwTick_Key_Set_Point = 0; // 控制key_proc的执行速度
char BMI_buffer[100];  // 定义一个足够大的字符数组以存储格式化结果
char adc_buffer[50];  // 用于存储格式化后的字符串

// 数据帧结束标志
unsigned char adc_t;
unsigned char DMA222;
// 全局变量
float adc_value;   //adc采集数据
int yy,zz,i,a_value=0;
//bmi
int bmi_len,adc_len;           // 存储实际发送的长度
int8_t bmi_id, CAN_ack=0;
uint16_t hg25q256_id = 0;

uint16_t time_tim7,time_out,time;

uint8_t rx_frame_index = 0;
uint8_t rx_frame[64]; // 存储完整帧

//按键状态和电源状态变量（volatile确保内存可见性）
volatile uint8_t key_state = 1;     // 按键状态：1=未按下，0=按下（默认未按下）
volatile uint8_t power_state = 1;   // 电源状态：1=开机，0=关机（默认开机）


const unsigned char *point;


extern volatile uint8_t ota_pending_request;
extern volatile ota_status_t ota_status;
extern can_ring_queue_t canQueue;


//函数宣告                
void SystemClock_Config(void);

uint8_t rbuf[128];

extern uint32_t g_log_current_addr;

uint8_t CANtxdata[8] = {11, 79, 79, 80, 66, 65, 67, 75};
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan);


void Log(uint8_t log_level,const char *fmt, ...) {
    char buf[128];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if(len > 0)
    {  
				//HAL_UART_Transmit(&huart1, (uint8_t*)buf, len, HAL_MAX_DELAY);
				
				uint8_t write_ret = Log_WriteToFlash_DMA(log_level, buf);
				if (write_ret  == 0) {
					printf("Log写入成功，addr:0x%X, 内容：%s\r\n", g_log_current_addr, buf);
					Log_UpdateWriteAddr();  //更新地址  
					
        } else if(write_ret  == 1){
					printf("参数错误\r\n");
          
        }else if(write_ret  == 2){
					printf("擦除失败\r\n");
				}else if(write_ret  == 3){
					printf("DMA写入失败\r\n");
				}else if(write_ret  == 4){
					printf("校验读取失败\r\n");
				}else if(write_ret  == 5){
					printf("读取超时处理\r\n");
				}else if(write_ret  == 6){
					printf("校验不通过\r\n");
				}
				
    }
}

int main(void)
{
	
	//重设向量表
	SCB->VTOR = 0x08004000;
	
  HAL_Init();
  SystemClock_Config();
	
	
	MX_UART5_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();  //交互板通信/UART/CAN
	MX_USART1_UART_Init();  //串口打印 
  MX_GPIO_Init();
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
	
	
	
	//电源使能
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_8,GPIO_PIN_SET);  //舵机供电使能
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_3,GPIO_PIN_SET); //adc使能
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_3,GPIO_PIN_SET); //开关机功能使能
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_6,GPIO_PIN_SET); //交互板供电使能
	//HAL_Delay(1000);//补充3秒开机时间
	
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,GPIO_PIN_SET);
	
	//Log存储初始化
	if (Log_StorageInit() != 0) {
			// 初始化失败处理
			printf("Log存储初始化失败\r\n");
	}

	//HAL_Delay(5000);
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
	
	const unsigned char  *point;
	point= &gImage_1[0];

	SPI1_SCK_0;
	SPI1_RST_0;
	HAL_Delay(1000);
	SPI1_RST_1;
	HAL_Delay(1000);
	TFT_init();
	Picture_Display(point);
	
	
	/*SPI1_RST_0;
	HAL_Delay(1000);
	SPI1_RST_1;
	HAL_Delay(1000);
	GC9D01_Init();

  // Draw a red pixel at (100, 100)
  GC9D01_DrawPixel(100, 100, 0xF800);

  // Fill the screen with blue
  //GC9D01_FillScreen(0x001F);*/

	
	HAL_TIM_Base_Start_IT(&htim7);//开始计时 
	
	//init_all_servos(&huart5);

	//HG25Q256_Write_Multiple_Pages(IMAGE1_FLASH_ADDR, gImage_1, 51200);
	
	// 从Flash读取并显示图片
	//ReadAndDisplayDataFromHG25Q256(IMAGE1_FLASH_ADDR);*/
	
	//DisplayAnimationFromFlash();
	
	printf("app1\r\n");
	
	
	//Log(0,"2222\r\n");
	//Log(0,"3333\r\n");
	/*uint8_t rbuf[256];
	uint32_t addr = 0x205000;
	uint8_t ret;
	memset(rbuf, 0, 256);
	dma_transfer_complete = 0;
	ret = HG25Q256_Read_Data_DMA(rbuf, 256, addr);
	if (ret != 0) {
			printf("DMA读启动失败, ret = %d\n", ret);
			
	}
	while (!dma_transfer_complete);
	printf("\n读取数据:\n");
	for (int i = 0; i < 256; i++) {
			printf("%02X ", rbuf[i]);
			if ((i + 1) % 16 == 0) printf("\n");
	}
	printf("\n");*/
	
	
	while (1)		
  {
	
		//HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_4);//指示灯闪烁
		
		if (g_trigger_log_send) {
				g_trigger_log_send = 0;  // 清除标志位
				
				// 发送所有日志到上位机
				printf("开始通过CAN发送Flash中的日志...\n");
				uint8_t ret = CAN_SendAllLogsFromFlash(LOG_FLASH_START, 100);  // 最多发送100条
				if (ret == 0) {
						printf("日志发送完成\n");
				} else {
						printf("日志发送失败，错误码：%d\n", ret);
				}
		}
		
	  if(uart1_rx_flag == 1)
		{
				// 数据处理：将串口1接收的数据转发到串口5
				memcpy(uart5_txbuf, uart1_rxbuf, strlen((char *)uart1_rxbuf));
				
				// 去除末尾的换行符（如果有）
				int len = strlen((char *)uart5_txbuf);
				if(len > 0 && uart5_txbuf[len-1] == '\n') {
						uart5_txbuf[len-1] = '\0'; // 去除换行符
						
						if(len > 1 && uart5_txbuf[len-2] == '\r') {
								uart5_txbuf[len-2] = '\0';
						}
				}
				//printf("uart5_txbuf:%s\r\n",uart5_txbuf);
				// 将字符串转换为十六进制数组
				int hex_length;
				string_to_hex(uart5_txbuf, (char *)uart1_rxbuf, &hex_length);
		
				
				if(hex_length > 0) {
						// 计算校验和并添加到帧尾
						uint8_t checksum = calculate_checksum(uart5_txbuf, hex_length);
						//printf("hex_length = %d\r\n",hex_length);
						//printf("checksum = 0x%02X\r\n",checksum);
						uart5_txbuf[hex_length-1] = checksum;
						
						// 打印发送的指令
						//printf("Sending to servo: ");
						//for(int i = 0; i <= hex_length; i++) {
								//printf("%02X ", uart5_txbuf[i]);
						//}
						//printf("\r\n");
						
						// 发送给舵机
						HAL_UART_Transmit(&huart5, uart5_txbuf, hex_length + 1, HAL_MAX_DELAY);
				}
				
				// 清空缓存与标志
				memset(uart1_rxbuf, 0, UART1_RX_BUF_SIZE);
				memset(uart5_txbuf, 0, UART5_TX_BUF_SIZE);
				uart1_rx_cnt = 0;
				uart1_rx_flag = 0;
				
				// 重新开启中断接收
				HAL_UART_Receive_IT(&huart1, &uart1_ch, 1);
		}

		//S20_Pro();
		
		//read_servo_current(&huart5, 2);
		
		//read_servo_angle(&huart5, 2);
		
    
		adc_value = getADC1() * 3.3 / 4096 * (8.4f / 2.8f);  //采集ADC数据
		adc_len = sprintf(adc_buffer, "voltage: %.2f V\r\n", adc_value);
		
		//printf("%s\r\n",adc_buffer);
		
		//bmi2_get_sensor_data(&sensor_data, &bmi);

		    // 格式化字符串，封装数据
    //bmi_len = sprintf(BMI_buffer, "ACC: X: %d Y: %d Z: %d\r\n"
                          //"GYRO: X: %d Y: %d Z: %d\r\n",
                          //sensor_data.acc.x, sensor_data.acc.y, sensor_data.acc.z,
                          //sensor_data.gyr.x, sensor_data.gyr.y, sensor_data.gyr.z);
		
		//printf("%s\r\n",BMI_buffer);
	
		
		process_can_queue(); // 实时处理队列
		// 队列清空后，判断是否允许OTA
    if (ota_pending_request && ota_status == OTA_PENDING && canQueue.size == 0) {
        ota_status = OTA_UPGRADING;
        ota_pending_request = 0;
				
    }
		//if(servo_status_check_flag)
    //{
        //servo_status_check_flag = 0; // 清除标志
        //check_servo_status(&huart5); // 检测堵转、温度等
    //}
		//adc_value = getADC1() * 3.3 / 4096 * (8.4f / 2.8f);  //采集ADC数据
		//adc_len = sprintf(adc_buffer, "voltage: %.2f V\r\n", adc_value);
		//printf("%s\r\n",adc_buffer);
		//Show_BatteryBar_From_ADC_New(adc_value);	
		HAL_Delay(10);
  }
}

int check_current(void)
{
	if(uart1_rx_flag == 1)
		{
				// 数据处理：将串口1接收的数据转发到串口5
				memcpy(uart5_txbuf, uart1_rxbuf, strlen((char *)uart1_rxbuf));
				
				// 去除末尾的换行符（如果有）
				int len = strlen((char *)uart5_txbuf);
				if(len > 0 && uart5_txbuf[len-1] == '\n') {
						uart5_txbuf[len-1] = '\0'; // 去除换行符
						
						if(len > 1 && uart5_txbuf[len-2] == '\r') {
								uart5_txbuf[len-2] = '\0';
						}
				}
				//printf("uart5_txbuf:%s\r\n",uart5_txbuf);
				// 将字符串转换为十六进制数组
				int hex_length;
				string_to_hex(uart5_txbuf, (char *)uart1_rxbuf, &hex_length);
		
				
				if(hex_length > 0) {
						// 计算校验和并添加到帧尾
						uint8_t checksum = calculate_checksum(uart5_txbuf, hex_length);
						//printf("hex_length = %d\r\n",hex_length);
						//printf("checksum = 0x%02X\r\n",checksum);
						uart5_txbuf[hex_length-1] = checksum;
						
						// 打印发送的指令
						//printf("Sending to servo: ");
						//for(int i = 0; i <= hex_length; i++) {
								//printf("%02X ", uart5_txbuf[i]);
						//}
						//printf("\r\n");
						
						// 发送给舵机
						HAL_UART_Transmit(&huart5, uart5_txbuf, hex_length + 1, HAL_MAX_DELAY);
				}
				
				// 清空缓存与标志
				memset(uart1_rxbuf, 0, UART1_RX_BUF_SIZE);
				memset(uart5_txbuf, 0, UART5_TX_BUF_SIZE);
				uart1_rx_cnt = 0;
				uart1_rx_flag = 0;
				
				// 重新开启中断接收
				HAL_UART_Receive_IT(&huart1, &uart1_ch, 1);
		}
	
}
// 检查长按关机
/*int check_shutdown(void)
{
   if((HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_2)==0)&&(i>=3)) // 长按3秒
		{
				i=0;
				if(power_on == 0) {
						HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET); // 开机
						power_on = 1;
						adc_value = getADC1() * 3.3 / 4096 * (8.4f / 2.8f);
				} else {
						HAL_NVIC_SystemReset(); 
						HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET); // 关机
						power_on = 0;
						adc_value = getADC1() * 3.3 / 4096 * (8.4f / 2.8f);
				}
				HAL_TIM_Base_Stop_IT(&htim6);
		}
		return 0;
}*/




// 发送完成回调，清除忙标志
void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
	

}

// UART接收中断回调函数
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)   //只有2号和5号舵机有应答
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

void SystemClock_Config(void)
{
	
	
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
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

  /** Initializes the CPU, AHB and APB buses clocks
  */
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


// 外部中断回调函数
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
   if (GPIO_Pin == GPIO_PIN_2) { 	
	  // 100ms软件消抖（避免机械抖动误触发）
    if((uwTick - uwTick_Key_Set_Point ) < 100)  return;	
			uwTick_Key_Set_Point = uwTick;

			// 读取当前按键电平，更新按键状态
			if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2) == 0)
			{
				// 按键按下（下降沿触发）：启动计时
				key_state = 0;          // 标记按键已按下
				i = 0;                  // 重置计时器（避免残留值）
				HAL_TIM_Base_Start_IT(&htim6); // 启动TIM6计时
			}
			else
			{
				// 按键释放（上升沿触发）：取消计时（仅当计时<3秒时）
				key_state = 1;          // 标记按键未按下
				if(i < 3)               // 若未到3秒，取消长按判断
				{
					i = 0;
					HAL_TIM_Base_Stop_IT(&htim6); // 停止计时
				}
			}
    }
		
}
	
//定时器中断
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance==TIM6)
	{
		i++;//每次TIM6中断发生时，计数器i加1，1秒中断一次
		
    if(i >= 3 && power_state == 1) // 计时满3秒，且当前处于开机状态 → 立即关机
    {
			HAL_NVIC_SystemReset(); 
      // 执行关机动作（切换GPIO_PIN_3状态，与原逻辑一致）
      HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3); 
      power_state = 0; // 更新电源状态为关机

      // 重置计时，避免重复触发
      i = 0;
      HAL_TIM_Base_Stop_IT(&htim6);
    }
		if(i >= 3 && power_state == 0)	// 计时满3秒，且当前处于关机状态 → 立即开机
		{
			// 执行关机动作（切换GPIO_PIN_3状态，与原逻辑一致）
      HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3); 
      power_state = 1; // 更新电源状态为关机

      // 重置计时，避免重复触发
      i = 0;
      HAL_TIM_Base_Stop_IT(&htim6);
		}
	}
	
	if(htim->Instance==TIM7)
	{
		servo_status_check_flag = 1; // 标志置位
		
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
			//printf("SPI1 transfer_complete\r\n");
    }
		if (hspi->Instance == SPI2) {
			dma_transfer_complete = 1;;  
    }
}

void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}


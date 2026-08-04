#include "includes.h"
#include "app.h"



//函数宣告                
void SystemClock_Config(void);
void Flash_WriteRead_Test(void);



void bootloader_start(void)
{
    /*==========打印消息==========*/
		uint32_t mode = read_start_mode();
		printf("\r\nBootloader read mode = 0x%08X (addr=0x%08X)\r\n", mode, OTA_FLAG_ADDR);
    switch(mode)										//读取是否启动应用程序
    {
			case STARTUP_NORMAL:										//正常启动
			{
				printf("\r\nnormal start......\r\n");
				//iap_execute_app(APPLICATION_1_ADDR);
				break;
			}
			case STARTUP_UPDATE:										//升级再启动
			{
				printf("\r\nstart update......\r\n");
				
				move_code(APPLICATION_1_ADDR,APPLICATION_2_ADDR,APPLICATION_2_SIZE);
				
				printf("> update success......\r\n");
				break;
			}
			case STARTUP_RESET:											//恢复出厂设置 目前没使用
			{
				printf("\r\nrestore to factory program......\r\n");
				break;
			}
			default:													//启动失败
			{
				printf("\r\nerror:%X!!!......\r\n", read_start_mode());
				return;
			}
    }

   // 添加应用程序验证
    uint32_t stack_pointer = *(__IO uint32_t*)APPLICATION_1_ADDR;
    uint32_t reset_vector = *(__IO uint32_t*)(APPLICATION_1_ADDR + 4);
    
    printf("\r\nApp Stack Ptr: 0x%08X, Reset Vector: 0x%08X\r\n", stack_pointer, reset_vector);
    
    // 检查栈指针是否有效
    if ((stack_pointer & 0x2FFE0000) != 0x20000000) {
        printf("\r\nERROR: Invalid stack pointer! Boot halted.\r\n");
        while(1);
    }
    
    // 检查复位向量是否在Flash范围内
    //if (reset_vector < 0x803FFFF || reset_vector > 0x8000000) {
        //printf("\r\nERROR: Invalid reset vector! Boot halted.\r\n");
        //while(1);
    //}

    printf("\r\nJumping to application at 0x%08X...\r\n", APPLICATION_1_ADDR);
    iap_execute_app(APPLICATION_1_ADDR);
}


void GD32_SwitchToPLL(void)
{
	#define RCU_CTL         (*(volatile uint32_t*)(0x40021000 + 0x00))
	#define RCU_CFG0        (*(volatile uint32_t*)(0x40021000 + 0x04))
	#define RCU_INT         (*(volatile uint32_t*)(0x40021000 + 0x08))
	#define RCU_APB2EN      (*(volatile uint32_t*)(0x40021000 + 0x18))
		
	
	// 1. 启动外部晶振（HSE）
	RCU_CTL |= (1 << 16);
	while (!(RCU_CTL & (1 << 17))); // 等待HSE稳定

	// 2. 配置PLL（HSE作为源，9倍频）
	RCU_CFG0 &= ~(0xF << 18); // 清除PLLMF
	RCU_CFG0 |= (7 << 18);    // PLLMF=9（0b0111）

	RCU_CFG0 &= ~(1 << 16);   // PLLSRC=HSE

	// 3. 启动PLL
	RCU_CTL |= (1 << 24);
	while (!(RCU_CTL & (1 << 25))); // 等待PLL稳定

	// 4. 切换SYSCLK到PLL
	RCU_CFG0 &= ~(3 << 0);
	RCU_CFG0 |= (2 << 0); // SW=PLL
	while ((RCU_CFG0 >> 2) & 0x3 != 0x2); // 等待切换完成
}
int main(void)
{

  HAL_Init();
  SystemClock_Config();
	//GD32_SwitchToPLL();
	// 设置FLASH等待周期
	//#define FMC_BASE        (0x40022000UL)
	//#define FMC_CTL0         (*(volatile uint32_t *)(FMC_BASE + 0x10))
	//FMC_CTL0 &= ~(7 << 0);  // 清空WAIT位
	//FMC_CTL0 |= (2 << 0);   // 设置2个等待周期
	
	MX_UART5_Init();
  MX_USART2_UART_Init();//串口打印
  MX_USART3_UART_Init();  //交互板通信/UART/CAN
	MX_USART1_UART_Init();   
  MX_GPIO_Init();
	MX_I2C2_Init();
	MX_TIM6_Init();
	MX_TIM7_Init();

	MX_CAN_Init();
	

	printf("SystemCoreClock: %lu\r\n", SystemCoreClock);
	//flash_rw_test();
	
	//Flash_EraseRange(0x08024000, 0x0803FFFF);
	bootloader_start();
	printf("here\r\n");
	
	
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


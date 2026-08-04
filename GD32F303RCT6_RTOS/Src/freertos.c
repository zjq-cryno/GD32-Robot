/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/

#include "main.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"
#include "adc.h"
#include "includes.h"
#include "app.h"
#include "can.h"

volatile uint8_t power_on = 0; // 0=关机，1=开机

volatile ota_status_t ota_status = OTA_IDLE;
uint32_t ota_total_len = 0;
volatile uint32_t ota_recv_len = 0;
volatile uint8_t ota_frame_cnt = 0;
volatile uint8_t ota_pending_request = 0;

osThreadId LedTaskHandle = NULL;
osThreadId CanTaskHandle = NULL;
osThreadId PowerOnOffTaskHandle = NULL;
osThreadId ServeTestTaskHandle = NULL;
osThreadId MonitorTaskHandle = NULL;
osMessageQId servoQueueHandle = NULL;
osThreadId sendLogHandle = NULL;
osThreadId ServoStallCheckTaskHandle = NULL;  // 堵转检测任务句柄
osThreadId ServoTestTaskHandle = NULL;  
osThreadId acquire_currentTaskHandle = NULL;
osThreadId servoTaskHandle = NULL;

void My_UART_Transmit(UART_HandleTypeDef *huart, uint8_t tx_frame[], int len);


void LedTaskFunc(void const * argument);
void ServeTestTaskFunc(void const * argument);
void ServoAngleSwitchTask(void const * argument);
void PowerOnOffTaskFunc(void const * argument);
void ServoStallCheckTaskFunc(void const * argument);
void CanTaskFunc(void const * argument);
void MonitorTaskFunc(void const * argument);
void sendLogFunc(void const * argument);
BaseType_t MyTaskHook(void *pvParameter);
void ServoTestTaskFunc(void const * argument);

void acquire_current(void const * argument);
void servoFunc(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */


void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );


static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  
}




void MX_FREERTOS_Init(void) {
	
	osMutexDef(uart5Mutex);
  uart5MutexHandle = osMutexCreate(osMutex(uart5Mutex));
	
	osMutexDef(g_mutex_printf);
  g_mutex_printf = osMutexCreate(osMutex(g_mutex_printf));
	
	osMutexDef(logMutex);
	logMutexHandle = osMutexCreate(osMutex(logMutex));
	
	osMessageQDef(servoQueue, 4, sizeof(ServoCommand));
  servoQueueHandle = osMessageCreate(osMessageQ(servoQueue), NULL);
	
	
	//osThreadDef(LedTask, LedTaskFunc, osPriorityIdle, 0, 128);
	//LedTaskHandle = osThreadCreate(osThread(LedTask), NULL);

	
	// 创建CAN接收队列
  canRxQueueHandle = xQueueCreate(10, sizeof(CanRxMsg));
	
	osThreadDef(CanTask, CanTaskFunc, osPriorityNormal, 0, 1024);
  CanTaskHandle = osThreadCreate(osThread(CanTask), NULL);
	
	//osThreadDef(acquire_currentTask, acquire_current, osPriorityNormal, 0, 256);
  //acquire_currentTaskHandle = osThreadCreate(osThread(acquire_currentTask), NULL);
	
	//osThreadDef(servoTask, servoFunc, osPriorityNormal, 0, 256);
  //servoTaskHandle = osThreadCreate(osThread(servoTask), NULL);
	
	//osThreadDef(sendLog, sendLogFunc, osPriorityNormal, 0, 512);
	//sendLogHandle = osThreadCreate(osThread(sendLog), NULL);
	
	
	//osThreadDef(PowerOnOffTask, PowerOnOffTaskFunc, osPriorityAboveNormal, 0, 128);
  //PowerOnOffTaskHandle = osThreadCreate(osThread(PowerOnOffTask), NULL);
	
	//osThreadDef(ServoTestTask, ServoTestTaskFunc, osPriorityNormal, 0, 128);
  //ServoTestTaskHandle = osThreadCreate(osThread(ServoTestTask), NULL);
	
	//osThreadDef(ServoStallCheckTask, ServoStallCheckTaskFunc, osPriorityNormal, 0, 256);
	//ServoStallCheckTaskHandle  = osThreadCreate(osThread(ServoStallCheckTask), NULL);
	
	//osThreadDef(ServoAngleSwitchTask, ServoAngleSwitchTask, osPriorityNormal, 0, 128);
  //osThreadCreate(osThread(ServoAngleSwitchTask), NULL);
		
	//osThreadDef(ServoCheckTask, ServoCheckTaskFunc, osPriorityNormal, 0, 256);
  //ServoCheckTaskHandle = osThreadCreate(osThread(ServoCheckTask), NULL);
	
	
	
	//osThreadDef(MonitorTask, MonitorTaskFunc, osPriorityNormal, 0, 128);
  //MonitorTaskHandle = osThreadCreate(osThread(MonitorTask), NULL);

}

void servoFunc(void const * argument)
{
		//servo_angle_speed(&huart5, BOTTOM, 0, 22, angle_speed[BOTTOM-1]);
		servo_angle_speed(&huart5, LEFT, 0, 60, angle_speed[LEFT-1]);
		//servo_angle_speed_Motor_mode(&huart5, LEFT, 0, 30);
	
		//servo_angle_speed(&huart5, RIGHT, 0, 60, angle_speed[RIGHT-1]);
		//servo_angle_speed(&huart5, NECK, 0, 22, angle_speed[NECK-1]);
		//servo_angle_speed(&huart5, HEAD_NOD, 0, 11, angle_speed[HEAD_NOD-1]);
		//servo_angle_speed(&huart5, HEAD_SHAKE, 0, 6, angle_speed[HEAD_SHAKE-1]);
		osDelay(2000);
		//servo_angle_speed(&huart5, BOTTOM, -45, 22, angle_speed[BOTTOM-1]);
		servo_angle_speed(&huart5, LEFT, -120, 60, angle_speed[LEFT-1]);
		//servo_angle_speed_Motor_mode(&huart5, LEFT, -120, 30);
	
		//servo_angle_speed(&huart5, RIGHT, -120, 60, angle_speed[RIGHT-1]);
		//servo_angle_speed(&huart5, NECK,-45, 22, angle_speed[NECK-1]);
		//servo_angle_speed(&huart5, HEAD_NOD, -22, 11, angle_speed[HEAD_NOD-1]);
		//servo_angle_speed(&huart5, HEAD_SHAKE, -12, 6, angle_speed[HEAD_SHAKE-1]);
		osDelay(2000);
		//servo_angle_speed(&huart5, BOTTOM, 0, 22, angle_speed[BOTTOM-1]);
		servo_angle_speed(&huart5, LEFT, 0, 60, angle_speed[LEFT-1]);
		//servo_angle_speed_Motor_mode(&huart5, LEFT, 0, 30);
		//servo_angle_speed(&huart5, RIGHT, 0, 60, angle_speed[RIGHT-1]);
		//servo_angle_speed(&huart5, NECK, 0, 22, angle_speed[NECK-1]);
		//servo_angle_speed(&huart5, HEAD_NOD, 0, 11, angle_speed[HEAD_NOD-1]);
		//servo_angle_speed(&huart5, HEAD_SHAKE, 0, 6, angle_speed[HEAD_SHAKE-1]);
		osDelay(2000);
		//servo_angle_speed(&huart5, BOTTOM, 45, 22, angle_speed[BOTTOM-1]);
		servo_angle_speed(&huart5, LEFT, 120, 60, angle_speed[LEFT-1]);
		//servo_angle_speed_Motor_mode(&huart5, LEFT, 120, 30);
		//servo_angle_speed(&huart5, RIGHT, 120, 60, angle_speed[RIGHT-1]);
		//servo_angle_speed(&huart5, NECK, 45, 22, angle_speed[NECK-1]);
		//servo_angle_speed(&huart5, HEAD_NOD, 22, 11, angle_speed[HEAD_NOD-1]);
		//servo_angle_speed(&huart5, HEAD_SHAKE, 12, 6, angle_speed[HEAD_SHAKE-1]);
		osDelay(2000);
}

void MonitorTaskFunc(void const * argument)
{
    for(;;)
    {
        uart1_print("任务状态监控:\r\n");
        
        // 检查舵机测试任务状态
        if(ServeTestTaskHandle != NULL)
        {
            osThreadState state = osThreadGetState(ServeTestTaskHandle);
            switch(state)
            {
                case osThreadReady: uart1_print("ServeTestTask: 就绪\r\n"); break;
                case osThreadRunning: uart1_print("ServeTestTask: 运行中\r\n"); break;
								case osThreadBlocked: uart1_print("ServeTestTask: 阻塞中\r\n"); break;
                case osThreadSuspended: uart1_print("ServeTestTask: 挂起中\r\n"); break;
                case osThreadDeleted: uart1_print("ServeTestTask: 被删除\r\n"); break;
                case osThreadError: uart1_print("ServeTestTask: 错误\r\n"); break;
                default: uart1_print("ServeTestTask: 未知状态 %d\r\n", state); break;
            }
        }
        
        // 检查空闲任务堆栈使用情况
        uart1_print("空闲任务堆栈剩余: %d bytes\r\n", uxTaskGetStackHighWaterMark(xTaskGetIdleTaskHandle()) * sizeof(StackType_t));
        
        osDelay(5000); // 每5秒检查一次
    }
}


void ServoTestTaskFunc(void const * argument)
{
		ServoCommand cmd;
    uint8_t servo_array[9] = {0xAB, 0x00, 0x09, 0x42, 0, 0, 0, 0, 0};
    UART_HandleTypeDef *huart = &huart5; // 舵机UART
    for (;;) {
        osEvent evt = osMessageGet(servoQueueHandle, osWaitForever);
        if (evt.status == osEventMessage) {
            cmd = *(ServoCommand*)evt.value.p;
            servo_angle_speed(huart, cmd.servo_id, cmd.angle, cmd.speed, servo_array);
        }
        // 无需osDelay，队列阻塞
    }
  
}

void acquire_current(void const * argument)
{
		for(;;)
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
			osDelay(500);
		}
	
}
// 定时动作发送任务，负责循环切换角度并发消息
void ServoAngleSwitchTask(void const * argument)
{
    // 角度表
    int16_t angles[] = {0, -45, 0, 45, 0};
    size_t angle_count = sizeof(angles)/sizeof(angles[0]);
    size_t idx = 0;
    ServoCommand cmd;
    cmd.servo_id = 1;  // 舵机ID，按你的硬件实际填写
    cmd.speed = 90;    // 速度，按需调整
	
    while (1) {
        cmd.angle = angles[idx];
        osMessagePut(servoQueueHandle, (uint32_t)&cmd, 0); // 发送动作
        idx = (idx + 1) % angle_count;
        osDelay(2000); // 每2秒切换一次
    }
}

// 定义一个钩子函数
BaseType_t MyTaskHook(void *pvParameter) {
    int *counter = (int *)pvParameter;
    (*counter)++;
    //uart1_print("Led Toggle %d times\r\n", *counter);
    return pdPASS;
}

void LedTaskFunc(void const * argument)
{
 
 int Counter = 0;
	uint8_t ucLargeBuffer[2048];  // 假设任务栈仅分配了1024字节
	// 给当前任务设置钩子函数
  vTaskSetApplicationTaskTag(NULL, MyTaskHook);
  for(;;)
  {
		 /*uint32_t start, elapsed_time;
    
    start_time(&start);
    
    osDelay(10);// 要测量时间的代码
    elapsed_time = stop_time(start);
		
		uart1_print("The code run time is %d ms\r\n",elapsed_time);*/
		
		HAL_GPIO_TogglePin(GPIOB,GPIO_PIN_4);//指示灯闪烁

		xTaskCallApplicationTaskHook(NULL, &Counter);		// 在任务里主动调用 Hook
		
    osDelay(1000);
  }
  
  
}

// 正常任务1 - 小栈空间（容易溢出）
void vTask1(void const * pvParameters)
{
    const char *taskName = "Task1";
    int stackArray[10]; // 小数组，容易溢出
    
    while(1)
    {
        printf("%s 运行中...\n", taskName);
        
        // 故意造成栈溢出的操作
        for(int i = 0; i < 100; i++)  // 越界访问
        {
            stackArray[i] = i;  // 这将导致栈溢出
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
void ServoStallCheckTaskFunc(void const *argument)
{

    // 循环查询状态
    for (;;) {
        for (uint8_t i = 1; i <= 6; i++) {  // 示例：查询ID1-6
            Servo_QueryStatus(i);
            osDelay(10);
        }
        osDelay(100);  // 100ms间隔
    }
}


void PowerOnOffTaskFunc(void const * argument)
{
	
    // 按键状态变量
    uint32_t key_press_time = 0;
    uint8_t key_cur_state;
    uint8_t debounce_counter = 0;

    
    // 状态机变量
    typedef enum {
        POWER_STATE_IDLE,        // 空闲状态
        POWER_STATE_DEBOUNCE,    // 消抖状态
        POWER_STATE_PRESSED,     // 按下状态
        POWER_STATE_LONG_PRESS,  // 长按状态
    } PowerState_t;
    
    PowerState_t power_state = POWER_STATE_IDLE;
    
    // 关键宏定义
    #define POWER_KEY_LONG_PRESS_MS  3000   // 3秒长按
    #define POWER_KEY_SCAN_INTERVAL  50     // 50ms扫描一次
    #define DEBOUNCE_TIME_MS        50      // 消抖时间50ms
    #define DEBOUNCE_THRESHOLD       (DEBOUNCE_TIME_MS / POWER_KEY_SCAN_INTERVAL) // 消抖计数阈值
    
    for(;;)
    {
				taskENTER_CRITICAL();    //禁止其他中断打断
        // 1. 读取按键状态
        key_cur_state = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_2);
        
        // 2. 状态机处理
        switch (power_state)
        {
            case POWER_STATE_IDLE:
                // 空闲状态，等待按键按下
                if (key_cur_state == 0) { // 下降沿触发，检测到按下
                    power_state = POWER_STATE_DEBOUNCE;
                    debounce_counter = 0;
                }
                break;
                
            case POWER_STATE_DEBOUNCE:
                // 消抖状态
                debounce_counter++;
                if (debounce_counter >= DEBOUNCE_THRESHOLD) {
                    // 消抖完成，确认按下
                    if (key_cur_state == 0) {
                        power_state = POWER_STATE_PRESSED;
                        key_press_time = 0;
                        
                    } else {
                        // 误触发，返回空闲状态
                        power_state = POWER_STATE_IDLE;
                    }
                }
                break;
                
            case POWER_STATE_PRESSED:
                // 按键按下状态，计时检测长按
                if (key_cur_state == 1) { // 按键提前释放
                    power_state = POWER_STATE_IDLE;
                    uart1_print("短按按键\r\n");
                } else {
                    key_press_time += POWER_KEY_SCAN_INTERVAL;
                    if (key_press_time >= POWER_KEY_LONG_PRESS_MS) {
                        power_state = POWER_STATE_LONG_PRESS;
                        uart1_print("检测到长按\r\n");
                    }
                }
                break;
                
            case POWER_STATE_LONG_PRESS:
                // 长按状态，执行开关机操作
                if (power_on == 0) {
										power_on = 1;
                    // 开机操作
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
                    
									
                    uart1_print("系统开机\r\n");
                    
                    // 读取ADC电压
                    adc_value = getADC1() * 3.3f / 4096.0f * (8.4f / 2.8f);
                } else {
                    // 关机操作
                    uart1_print("检测到长按3秒，立即关机...\r\n");
										
                    // 1. 关闭所有外设电源
                    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);  // 舵机供电关闭
                    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET);  // ADC使能关闭
                    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);  // 交互板供电关闭
                    
                    // 2. 关闭主电源
                    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
                    
                    // 3. 更新ADC电压
                    adc_value = getADC1() * 3.3f / 4096.0f * (8.4f / 2.8f);
                    
                    // 4. 强制系统复位（关键新增：无需等待按键松手，立即重启）
										
										HAL_NVIC_SystemReset();// 触发强制复位（等效“立即关机重启”）
                }
                break;
            default:
                power_state = POWER_STATE_IDLE;
                break;
        }
				taskEXIT_CRITICAL();  // 必须先退出临界区，避免复位时中断异常
        
        osDelay(POWER_KEY_SCAN_INTERVAL);
    }
}


void CanTaskFunc(void const * argument)
{
		 CanRxMsg rxFrame;
    BaseType_t xStatus;
    uint8_t calc_chk, recv_chk;
    uint32_t can_id;
    uint8_t tx_frame[7];
	
    static uint32_t cur_addr = OTA_FLASH_BASE;
		
		TickType_t xLastWakeTime;						// 上次唤醒时间
		const TickType_t xFrequency = 10;		// 执行频率（10个滴答）
		xLastWakeTime = xTaskGetTickCount();
    
    for(;;) {
        // 等待CAN消息 - 使用FreeRTOS原生队列API
        if(xQueueReceive(canRxQueueHandle, &rxFrame, portMAX_DELAY) == pdTRUE) {
            calc_chk = xor_checksum(rxFrame.data);
            recv_chk = rxFrame.data[7];
            can_id = rxFrame.id;
            uint8_t frame_cnt = rxFrame.data[6];
            if (calc_chk != recv_chk) {
                CAN_Send_Ack(can_id, 0xFF,frame_cnt);
                continue;   
            }
            
            // 处理不同类型的CAN消息
            switch(can_id)
						{
							case 0x100:
							case 0x101:
							case 0x102:
							{
                // 处理舵机控制消息
                uint8_t servo_id = rxFrame.data[1];
                int8_t angle_data1 = rxFrame.data[2];
                int8_t angle_data2 = rxFrame.data[3];
                uint8_t speed_data1 = rxFrame.data[4];
                uint8_t speed_data2 = rxFrame.data[5];
                
                angle_speed[servo_id - 1][1] = servo_id;
                angle_speed[servo_id - 1][4] = angle_data1;
                angle_speed[servo_id - 1][5] = angle_data2;
                angle_speed[servo_id - 1][6] = speed_data1;
                angle_speed[servo_id - 1][7] = speed_data2;
                angle_speed[servo_id - 1][8] = calculate_checksum(angle_speed[servo_id - 1], 9);
                
								//Log(0,"servo %d 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n", servo_id, angle_speed[servo_id - 1][0],angle_speed[servo_id - 1][1],angle_speed[servo_id - 1][2],
								//angle_speed[servo_id - 1][3],angle_speed[servo_id - 1][4],angle_speed[servo_id - 1][5],angle_speed[servo_id - 1][6],angle_speed[servo_id - 1][7],angle_speed[servo_id - 1][8]);  
								//Log(0,"----------------------\r\n");
                
                // 组装串口帧并发送
                tx_frame[0] = 0xAB;
                tx_frame[1] = servo_id;
                tx_frame[2] = 0x07;
                tx_frame[3] = 0x47;
                tx_frame[4] = (can_id == 0x100 ? 0 : can_id == 0x101 ? 1 : 2);
                tx_frame[5] = 0;
                tx_frame[6] = calculate_checksum(tx_frame, 7);
                

                My_UART_Transmit(&huart5, tx_frame, 7);   
								
                My_UART_Transmit(&huart5, angle_speed[servo_id - 1], 9);
								
                uint16_t dida_idx = 0;

								while(dida_idx++<400 && update_servo[servo_id-1]==false){
									osDelay(1);
								}
								if(update_servo[servo_id-1]==true){
									CAN_Send_Ack(can_id, 0x00,frame_cnt); // 0x00为正确应答
									update_servo[servo_id-1] = false;
								}else{
									printf("time out\r\n");
									CAN_Send_Ack(can_id, 0xFF,frame_cnt); // 0xFF为错误应答
									//update_servo[servo_id-1] = false;
								}
							}break;
							case 0x103: {
                // 处理位置查询消息
                uint8_t servo_id = rxFrame.data[1];
                uint8_t tx_frame[5] = {0xAB, servo_id, 0x05, 0x51};
                tx_frame[4] = calculate_checksum(tx_frame, 5);
                
                My_UART_Transmit(&huart5, tx_frame, 5);
								//CAN_Send_pos_Ack(can_id, position, 0x00, frame_cnt);
								
                uint8_t dida_idx = 0;
								while(dida_idx++<150 && update_servo[servo_id-1]==false){
									osDelay(1);
								}
								if(update_servo[servo_id-1]==true){
									printf("position is %d\r\n",position);
									CAN_Send_pos_Ack(can_id, position, 0x00, frame_cnt);
									update_servo[servo_id-1] = false;
								}else{
									printf("time out");
									CAN_Send_pos_Ack(can_id, position, 0xFF, frame_cnt);
									//update_servo[servo_id-1] = false;
								}
							}break;
							if(can_id == 0x104)    //修改舵机扭矩状态
							{
								uint8_t servo_id = rxFrame.data[1];
								uint8_t torque_value = rxFrame.data[2];
								uint8_t tx_frame[6] = {0xAB, servo_id, 0x06, 0x40, torque_value};
								tx_frame[5] = calculate_checksum(tx_frame, 6);
								// 发送命令
								HAL_UART_Transmit(&huart5, tx_frame, 6, HAL_MAX_DELAY);
								
								CAN_Send_Ack(can_id, 0x00,frame_cnt);
								/*uint16_t dida_idx = 0;
								while(dida_idx++<400 && update_servo[servo_id-1]==false){
									osDelay(1);
								}
								if(update_servo[servo_id-1]==true){
									CAN_Send_Ack(can_id, 0x00,frame_cnt); // 0x00为正确应答
									update_servo[servo_id-1] = false;
								}else{
									printf("time out");
									CAN_Send_Ack(can_id, 0xFF,frame_cnt); // 0xFF为错误应答
									update_servo[servo_id-1] = false;
								}*/
							}
							if(can_id == 0x105)		//查询舵机当前电流
							{
								uint8_t servo_id = rxFrame.data[1];
								uint8_t tx_frame[5] = {0xAB, servo_id, 0x05, 0x53};
					
								// 计算校验和并添加到命令末尾
								tx_frame[4] = calculate_checksum(tx_frame, 5);
								
								My_UART_Transmit(&huart5, tx_frame, 5);
								// 发送命令
								//HAL_UART_Transmit(&huart5, tx_frame, 5, HAL_MAX_DELAY);
								
								
								uint8_t dida_idx = 0;

								while(dida_idx++<250 && update_servo[servo_id-1]==false){
									osDelay(1);
								}
								if(update_servo[servo_id-1]==true){
									
									CAN_Send_Current_Ack(can_id, cur_current, 0x00, frame_cnt);
									printf("cur_current is %d A\r\n",cur_current/10);
									update_servo[servo_id-1] = false;
								}else{
									printf("time out");
									CAN_Send_Current_Ack(can_id, cur_current, 0xFF, frame_cnt);
									update_servo[servo_id-1] = false;
								}

							}	
							case 0x401:{
								//vTaskSuspendAll();  //挂起其他任务，避免被其它任务打断
								 
                // 处理OTA请求帧
                ota_recv_len = 0;
                ota_total_len = ((uint32_t)rxFrame.data[1] << 24) | 
                                ((uint32_t)rxFrame.data[2] << 16) | 
                                ((uint32_t)rxFrame.data[3] << 8) | 
                                rxFrame.data[4];
                
               // Log(0,"ota_total_len is %u\r\n", ota_total_len);
								uart1_print("ota_total_len is %u\r\n", ota_total_len);
                
                if (ota_total_len == 0 || ota_total_len > OTA_FLASH_MAX_SIZE) {
                    //Log(0,"ota error\r\n");
										uart1_print("ota error\r\n");
                    CAN_Send_Ack(can_id, 0xFF,frame_cnt);
                    ota_status = OTA_IDLE;
                    continue;
                }
                
                if (ota_status == OTA_IDLE) {        
                    ota_status = OTA_UPGRADING;
                    //ota_pending_request = 1;
                    CAN_Send_Ack(can_id, 0x00,frame_cnt);
                } else {
                    CAN_Send_Ack(can_id, 0xFF,frame_cnt);
                }
							}break;
							case 0x402:{
								if(ota_status == OTA_UPGRADING) {
                // 处理OTA数据帧
									uint8_t remain = rxFrame.data[0];
									int datalen = (ota_total_len - ota_recv_len >= 4) ? 4 : (ota_total_len - ota_recv_len);
									int left = ota_total_len - ota_recv_len;
									datalen = (left >= 4) ? 4 : left;
									
									if (datalen == 4) {
											uint8_t buf[4] = {0xFF, 0xFF, 0xFF, 0xFF};
											memcpy(buf, &rxFrame.data[1], 4);
											ota_flash_write(cur_addr, buf, 4);
											ota_recv_len += datalen;
											cur_addr += 4;
									}
                
									if(datalen > 0 && datalen < 4) {
                    uint8_t buf[4] = {0xFF, 0xFF, 0xFF, 0xFF};
                    memcpy(buf, &rxFrame.data[1], left);
                    ota_flash_write(cur_addr, buf, 4);
                    ota_recv_len += datalen;
									}
                 
									if (ota_recv_len < ota_total_len) {
                    CAN_Send_Ack(can_id, 0x00,frame_cnt);
                    //Log(0,"ota_recv_len: %d\r\n", ota_recv_len);
										uart1_print("ota_recv_len: %d\r\n", ota_recv_len);
									} else {
                    ota_status = OTA_IDLE;
                    Log(0,"ota finish, ota_recv_len = %u\r\n", ota_recv_len);
										//uart1_print("ota finish, ota_recv_len = %u\r\n", ota_recv_len);
                    CAN_Send_Ack(can_id, 0xFE,frame_cnt);
                    
                    uint32_t last_word = read_start_mode();
                    write_start_mode(STARTUP_UPDATE);
                    
                    vTaskDelay(pdMS_TO_TICKS(10000));
									
                    HAL_NVIC_SystemReset();
										//xTaskResumeAll();				//恢复任务调度器
									}
								}
            }break;
        }
        
        // 等待下一个周期
        vTaskDelayUntil( &xLastWakeTime, xFrequency );		//固定每10个滴答轮询一次该任务
    }
	}
}

void sendLogFunc(void const * argument)
{
	for(;;)
	{
		if (g_trigger_log_send) {
				g_trigger_log_send = 0;  // 清除标志位
				
				// 发送所有日志到上位机
				uart1_print("开始通过CAN发送Flash中的日志...\n");
				uint8_t ret = CAN_SendAllLogsFromFlash(LOG_FLASH_START, 100);  // 最多发送100条
				if (ret == 0) {
						uart1_print("日志发送完成\n");
				} else {
						uart1_print("日志发送失败，错误码：%d\n", ret);
				}
		}
		osDelay(10);
	}
}
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{

    uart1_print("!!!!!!!!! 栈溢出检测 !!!!!!!!!\r\n");
    uart1_print("发生溢出的任务:%s\r\n", pcTaskName);
    uart1_print("系统复位...\r\n");
    
    HAL_NVIC_SystemReset();
    
}

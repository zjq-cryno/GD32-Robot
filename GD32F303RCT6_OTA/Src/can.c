/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    can.c
  * @brief   This file provides code for the configuration
  *          of the CAN instances.
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
#include "app.h"
#include "includes.h"

/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

CAN_HandleTypeDef hcan;

CAN_TxHeaderTypeDef txHeader;
CAN_RxHeaderTypeDef rxHeader;

uint8_t error_frame_num=0;
volatile uint8_t recv_error_flag=0;

/*******************CAN回环测试***********************/
uint8_t txdata[8] = {76, 79, 79, 80, 66, 65, 67, 75};
uint32_t TxID = 0x100;
uint8_t TxLength = 3;
uint8_t rxdata[8] = {0, 0, 0, 0, 0, 0, 0, 0};

uint32_t RxID;
uint8_t RxLength;
uint8_t CAN1_Send_Msg(uint32_t id, uint8_t *msg, uint8_t len);
uint8_t CAN1_Recv_Msg(uint32_t *ID, uint8_t *buf, uint8_t *Length);
/*******************CAN回环测试***********************/

volatile ota_status_t ota_status = OTA_IDLE;
uint32_t ota_total_len = 0;
volatile uint32_t ota_recv_len = 0;
volatile uint8_t ota_frame_cnt = 0;
volatile uint8_t ota_pending_request = 0;
// 用于缓存一帧
uint8_t ota_frame_buf[8];
can_ring_queue_t canQueue = {0};

/* CAN init function */
void MX_CAN_Init(void)
{

  /* USER CODE BEGIN CAN_Init 0 */

  /* USER CODE END CAN_Init 0 */

  /* USER CODE BEGIN CAN_Init 1 */
  /* USER CODE END CAN_Init 1 */
  hcan.Instance = CAN1;
  hcan.Init.Prescaler = 6;    //4              500kbps
  hcan.Init.Mode = CAN_MODE_NORMAL;
	//hcan.Init.Mode = CAN_MODE_LOOPBACK;//CAN_MODE_NORMAL
  hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan.Init.TimeSeg1 = CAN_BS1_8TQ;//5
  hcan.Init.TimeSeg2 = CAN_BS2_3TQ;//3
  hcan.Init.TimeTriggeredMode = DISABLE;
  hcan.Init.AutoBusOff = ENABLE;
  hcan.Init.AutoWakeUp = ENABLE;
  hcan.Init.AutoRetransmission = DISABLE;
  hcan.Init.ReceiveFifoLocked = DISABLE;
  hcan.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan) != HAL_OK)
  {
    Error_Handler();
  }
	
	
	
  /* USER CODE BEGIN CAN_Init 2 */
    CAN_FilterTypeDef sFilterConfig;
    
    sFilterConfig.FilterBank = 0;                               // 选择过滤器银行
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;           // ID 遮罩模式
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;          // 32 位过滤器
		sFilterConfig.FilterIdHigh = 0x0000;              // 高位过滤器 ID
		sFilterConfig.FilterIdLow = 0x0000;                         // 低位过滤器 ID
		sFilterConfig.FilterMaskIdHigh = 0x0000;                    // 允许所有ID
		sFilterConfig.FilterMaskIdLow = 0x0000;                     // 低位掩码
    sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;       // 将过滤器分配到 FIFO 0  
    sFilterConfig.FilterActivation = CAN_FILTER_ENABLE;                     // 启用过滤器
		sFilterConfig.SlaveStartFilterBank  =14;
    // 配置过滤器
    if (HAL_CAN_ConfigFilter(&hcan, &sFilterConfig) != HAL_OK)
    {
        // 过滤器配置错误处理
        Error_Handler();
    }
		HAL_CAN_Start(&hcan);
		HAL_CAN_ActivateNotification(&hcan,CAN_IT_RX_FIFO0_MSG_PENDING);

}

void HAL_CAN_MspInit(CAN_HandleTypeDef* canHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(canHandle->Instance==CAN1)
  {
  /* USER CODE BEGIN CAN1_MspInit 0 */

  /* USER CODE END CAN1_MspInit 0 */
    /* CAN1 clock enable */
    __HAL_RCC_CAN1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**CAN GPIO Configuration
    PA11     ------> CAN_RX
    PA12     ------> CAN_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* CAN1 interrupt Init */
    HAL_NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
  /* USER CODE BEGIN CAN1_MspInit 1 */

  /* USER CODE END CAN1_MspInit 1 */
  }
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef* canHandle)
{

  if(canHandle->Instance==CAN1)
  {
  /* USER CODE BEGIN CAN1_MspDeInit 0 */

  /* USER CODE END CAN1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_CAN1_CLK_DISABLE();

    /**CAN GPIO Configuration
    PA11     ------> CAN_RX
    PA12     ------> CAN_TX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11|GPIO_PIN_12);

    /* CAN1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USB_LP_CAN1_RX0_IRQn);
  /* USER CODE BEGIN CAN1_MspDeInit 1 */

  /* USER CODE END CAN1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
uint8_t CAN1_Send_Msg(uint32_t id, uint8_t *msg, uint8_t len){
	uint16_t i = 0;
	uint32_t txMailBox;
	uint8_t send_buf[8];
	
	txHeader.StdId = id;
	txHeader.ExtId = id;
	txHeader.IDE = CAN_ID_STD;
	txHeader.RTR = CAN_RTR_DATA;
	txHeader.DLC = len;
	
	for(i = 0; i < len; i++)
		send_buf[i] = msg[i];
	// 调试：打印即将发送的帧信息
  //printf("[DEBUG] Preparing to send frame:\r\n");
  //printf("  CAN ID: %d, Length: %d\r\n", txHeader.StdId, txHeader.DLC);
  //printf("  Data: ");
  //for(int i = 0; i < txHeader.DLC; i++) {
    //printf("%02X ", send_buf[i]);
  //}
  
	if(HAL_CAN_AddTxMessage(&hcan, &txHeader, send_buf, &txMailBox) != HAL_OK)
		return 1;	
	return 0;
}

uint8_t CAN1_Recv_Msg(uint32_t *ID, uint8_t *buf, uint8_t *Length){
	uint16_t i = 0;	

	if(HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0) > 0) {

		if(HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &rxHeader, buf) == HAL_OK){
		
			if(rxHeader.IDE == CAN_ID_STD)
			{
				*ID = rxHeader.StdId;
				//printf("StdId ID: %d\r\n", rxHeader.StdId);
			}
			else
			{
				*ID = rxHeader.ExtId;
				//printf("ExtId ID: %d\r\n", rxHeader.ExtId);
			}
			
			printf("CAN IDE: %d\r\n", rxHeader.IDE);
			printf("CAN RTR: %d\r\n", rxHeader.RTR);
			printf("CAN DLC: %d\r\n", rxHeader.DLC);
			printf("Recv Data: ");
			
			for(i = 0; i < rxHeader.DLC; i++)
				printf("0x%02X	",buf[i]);
			
			printf("\n");	
			//对一帧数据的前七个数异或校验
			uint8_t xor_val = 0;
			for (int i = 0; i < rxHeader.DLC-1; i++) {
					xor_val ^= buf[i];
			}
			if(xor_val == buf[rxHeader.DLC]){	
				//printf("校验正确\r\n");
				
			}else{
				//printf("校验错误\r\n");
				recv_error_flag=1;
				//记录错误帧计次
				error_frame_num=buf[rxHeader.DLC-2];
			}
		}
		*Length = rxHeader.DLC;   // 重要！返回长度
		return rxHeader.DLC;
	}
	return 0;
}

uint8_t CAN1_Loopback(void)
{
		int ret;
 		printf("开始CAN回环测试\r\n");
		ret = CAN1_Send_Msg(TxID,txdata, 8);
		// 发送后检查错误状态
		if(hcan.ErrorCode != HAL_CAN_ERROR_NONE) {
				printf("CAN错误码: %d\r\n", hcan.ErrorCode);
		}
		if(ret == 0)
			printf("CAN Send success!\r\n");
		else 
			printf("CAN Send failed!\r\n");
		
		CAN1_Recv_Msg(&RxID,rxdata,&RxLength);	
		printf("+++++++++++++++++++++++++++++++\r\n");
		return ret;
}
// 异或校验
uint8_t xor_checksum(uint8_t *data) {
    uint8_t res = 0;
    for (int i = 0; i < 7; i++) {
        res ^= data[i];
    }
    return res;
}
// 主循环处理
void process_can_queue(void)
{
    can_frame_node_t pkt;
		static uint32_t cur_addr = OTA_FLASH_BASE;
    while (can_queue_pop(&canQueue, &pkt))
    {			
				uint8_t *frame = pkt.frame;
        uint8_t frame_cnt = frame[6];
        uint8_t recv_chk = frame[7];
        uint8_t calc_chk = xor_checksum(frame);
				uint32_t can_id = pkt.can_id;
			
        uint8_t tx_frame[16];
				// 校验失败立即NACK
        if (calc_chk != recv_chk) {
            CAN_Send_Ack(can_id, 0xFF,frame_cnt);
            continue;
        }
				if(can_id == 0x100 || can_id == 0x101 || can_id == 0x102) {
					
						
						uint8_t servo_id = pkt.frame[1];
						int8_t angle_data1 = pkt.frame[2];
						int8_t angle_data2 = pkt.frame[3];
						uint8_t speed_data1 = pkt.frame[4];
						uint8_t speed_data2 = pkt.frame[5];
						angle_speed[servo_id - 1][1] = servo_id;
						angle_speed[servo_id - 1][4] = angle_data1;
						angle_speed[servo_id - 1][5] = angle_data2;
						angle_speed[servo_id - 1][6] = speed_data1;
						angle_speed[servo_id - 1][7] = speed_data2;
						angle_speed[servo_id - 1][8] = calculate_checksum(angle_speed[servo_id - 1], 9);
						//Log(0,"servo %d 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n", servo_id, angle_speed[servo_id - 1][0],angle_speed[servo_id - 1][1],angle_speed[servo_id - 1][2],
							//angle_speed[servo_id - 1][3],angle_speed[servo_id - 1][4],angle_speed[servo_id - 1][5],angle_speed[servo_id - 1][6],angle_speed[servo_id - 1][7],angle_speed[servo_id - 1][8]);  
						//Log(0,"----------------------\r\n");
						// 组装两条串口帧
						tx_frame[0] = 0xAB;
						tx_frame[1] = servo_id;
						tx_frame[2] = 0x07;
						tx_frame[3] = 0x47;
						tx_frame[4] = (can_id == 0x100 ? 0 : can_id == 0x101 ? 1 : 2);
						tx_frame[5] = 0;
						tx_frame[6] = calculate_checksum(tx_frame, 7);

						HAL_StatusTypeDef ret = HAL_UART_Transmit(&huart5, tx_frame, 7, HAL_MAX_DELAY);
						if(ret == HAL_OK){
							printf("HAL_UART_Transmit HAL_OK,Line=%d\r\n",__LINE__);
						}else if(ret == HAL_ERROR){						
							printf("HAL_UART_Transmit HAL_ERROR,Line=%d\r\n",__LINE__);
						}else if(ret == HAL_BUSY){
							printf("HAL_UART_Transmit HAL_BUSY,Line=%d\r\n",__LINE__);
						}else if(ret == HAL_TIMEOUT){
							printf("HAL_UART_Transmit HAL_TIMEOUT,Line=%d\r\n",__LINE__);
						}
						HAL_Delay(10);
						ret = HAL_UART_Transmit(&huart5, angle_speed[servo_id - 1], 9, HAL_MAX_DELAY);
						if(ret == HAL_OK){
							printf("HAL_UART_Transmit HAL_OK,Line=%d\r\n",__LINE__);
						}else if(ret == HAL_ERROR){
							printf("HAL_UART_Transmit HAL_ERROR,Line=%d\r\n",__LINE__);
						}else if(ret == HAL_BUSY){
							printf("HAL_UART_Transmit HAL_BUSY,Line=%d\r\n",__LINE__);
						}else if(ret == HAL_TIMEOUT){
							printf("HAL_UART_Transmit HAL_TIMEOUT,Line=%d\r\n",__LINE__);
						}
						//CAN_Send_Ack(pkt.can_id, 0x00,frame_cnt);
						uint16_t dida_idx = 0;
						while(dida_idx++<400 && update_servo[servo_id-1]==false){
							HAL_Delay(1);
						}
						if(update_servo[servo_id-1]==true){
							CAN_Send_Ack(pkt.can_id, 0x00,frame_cnt); // 0x00为正确应答
							update_servo[servo_id-1] = false;
						}else{
							printf("time out");
							CAN_Send_Ack(pkt.can_id, 0xFF,frame_cnt); // 0xFF为错误应答
							update_servo[servo_id-1] = false;
						}
						
				}
	
				if(can_id == 0x103)
				{
					uint8_t servo_id = pkt.frame[1];
					uint8_t tx_frame[5] = {0xAB, servo_id, 0x05, 0x51};
		
					// 计算校验和并添加到命令末尾
					tx_frame[4] = calculate_checksum(tx_frame, 5);

					// 发送命令
					HAL_UART_Transmit(&huart5, tx_frame, 5, HAL_MAX_DELAY);
					uint8_t dida_idx = 0;

					while(dida_idx++<150 && update_servo[servo_id-1]==false){
						HAL_Delay(1);
					}
					if(update_servo[servo_id-1]==true){
						printf("position is %d\r\n",position);
						CAN_Send_pos_Ack(can_id, position, 0x00, frame_cnt);
						update_servo[servo_id-1] = false;
					}else{
						printf("time out");
						CAN_Send_pos_Ack(can_id, position, 0xFF, frame_cnt);
						update_servo[servo_id-1] = false;
					}

				}	
				if(can_id == 0x104)    //修改舵机扭矩状态
				{
					uint8_t servo_id = pkt.frame[1];
					uint8_t torque_value = pkt.frame[2];
					uint8_t tx_frame[6] = {0xAB, servo_id, 0x06, 0x40, torque_value};
					tx_frame[5] = calculate_checksum(tx_frame, 6);
					// 发送命令
					HAL_UART_Transmit(&huart5, tx_frame, 6, HAL_MAX_DELAY);
					
					CAN_Send_Ack(pkt.can_id, 0x00,frame_cnt);
					/*uint16_t dida_idx = 0;
					while(dida_idx++<400 && update_servo[servo_id-1]==false){
						HAL_Delay(1);
					}
					if(update_servo[servo_id-1]==true){
						CAN_Send_Ack(pkt.can_id, 0x00,frame_cnt); // 0x00为正确应答
						update_servo[servo_id-1] = false;
					}else{
						printf("time out");
						CAN_Send_Ack(pkt.can_id, 0xFF,frame_cnt); // 0xFF为错误应答
						update_servo[servo_id-1] = false;
					}*/
				}
				if(can_id == 0x105)		//查询舵机当前电流
				{
					uint8_t servo_id = pkt.frame[1];
					uint8_t tx_frame[5] = {0xAB, servo_id, 0x05, 0x53};
		
					// 计算校验和并添加到命令末尾
					tx_frame[4] = calculate_checksum(tx_frame, 5);

					// 发送命令
					HAL_UART_Transmit(&huart5, tx_frame, 5, HAL_MAX_DELAY);
					
					
					uint8_t dida_idx = 0;

					while(dida_idx++<250 && update_servo[servo_id-1]==false){
						HAL_Delay(1);
					}
					if(update_servo[servo_id-1]==true){
						
						CAN_Send_Current_Ack(can_id, cur_current, 0x00, frame_cnt);
						//printf("cur_current is %d A\r\n",cur_current/10);
						update_servo[servo_id-1] = false;
					}else{
						printf("time out");
						CAN_Send_Current_Ack(can_id, cur_current, 0xFF, frame_cnt);
						update_servo[servo_id-1] = false;
					}

				}	
				// ===== 1. OTA请求帧 =====
				if (can_id == 0x401) {
					ota_recv_len = 0;
					// 解析文件长度
					ota_total_len = ((uint32_t)frame[1] << 24)|((uint32_t)frame[2] << 16)|((uint32_t)frame[3] << 8)|frame[4];
					Log(0,"ota_total_len is %u\r\n", ota_total_len);
					if (ota_total_len == 0 || ota_total_len > OTA_FLASH_MAX_SIZE) {
								printf("ota error\r\n");
								CAN_Send_Ack(can_id, 0xFF,frame_cnt);
								ota_status = OTA_IDLE;
								continue;
					}
					if (ota_status == OTA_IDLE) {		
							ota_status = OTA_PENDING;       // 等业务队列处理完再应答
							ota_pending_request = 1;
							CAN_Send_Ack(can_id, 0x00,frame_cnt); // ACK
							//g_ota_last_progress = 0;  // 重置进度记录
              //TFT_Display_OTA_Progress(0);  // 显示初始进度
					} else {
							CAN_Send_Ack(can_id, 0xFF,frame_cnt); // NACK (忙)
					}
					Log(0,"canQueue.size=%d\r\n",canQueue.size);
					Log(0,"ota_pending_request=%d\r\n",ota_pending_request);
					Log(0,"ota_status=%d\r\n",ota_status);
					continue;
				}				

				// ===== 2. OTA后续帧 =====
				if (can_id == 0x402 && ota_status == OTA_UPGRADING) {
						uint8_t remain = frame[0];
					//后续帧frame[0]为剩余字节，frame[1、2、3、4]为ota数据，frame[5]:无效数据 frame[6]为帧计次，frame[7]为校验
						int datalen = (ota_total_len - ota_recv_len >= 4) ? 4 : (ota_total_len - ota_recv_len);
						int left = ota_total_len - ota_recv_len;  //判断最后一帧
						datalen = (left >= 4) ? 4 : left;
						if (datalen == 4) {
								uint8_t buf[4] = {0xFF, 0xFF, 0xFF, 0xFF};
								memcpy(buf,&frame[1],4);
								
								ota_flash_write(cur_addr, buf, 4);  //写入4个uint8_t数据
								ota_recv_len += datalen;
								cur_addr = cur_addr + 4;  //写完后地址偏移
						}
						
						
						if(datalen > 0 && datalen < 4)  //最后一帧不足4字节
						{
							uint8_t buf[4] = {0xFF, 0xFF, 0xFF, 0xFF};
							memcpy(buf,&frame[1],left);
							ota_flash_write(cur_addr, buf, 4);  //写入4个uint8_t数据
							ota_recv_len += datalen;
							//cur_addr = cur_addr + datalen;
						}
						
						//uint8_t current_progress = (ota_recv_len * 100) / ota_total_len;
            //TFT_Display_OTA_Progress(current_progress);  // 实时更新进度
						
						if (ota_recv_len < ota_total_len) {
							CAN_Send_Ack(can_id, 0x00,frame_cnt);
							Log(0,"ota_recv_len:	%d\r\n",ota_recv_len);
						} else {
								ota_status = OTA_IDLE;
								Log(0,"ota finish, ota_recv_len = %u\r\n", ota_recv_len);
								CAN_Send_Ack(can_id, 0xFE,frame_cnt); // 0xFE=升级完毕
								//printf("OTA bin end addr = 0x%08X, flag addr = 0x%08X\r\n", OTA_FLASH_BASE + ota_total_len - 1, FLAG_ADDR);
								
								/*for (uint32_t addr = OTA_FLASH_BASE; addr < OTA_FLASH_BASE + ota_total_len; addr += 4) {
									uint32_t expected = 0;
									
									
									uint32_t actual = fmc_read_word(addr);
									
									Log(0,"addr: 0x%08X, actual data: 0x%08X\r\n", addr,actual);
									
									
								}*/
								// 读取最后4字节验证
								uint32_t last_word = read_start_mode();
								Log(0,"Before last 4 bytes: 0x%08X\r\n", last_word);
								write_start_mode(STARTUP_UPDATE);
								
								//uint32_t stack_pointer = *(__IO uint32_t*)APPLICATION_2_ADDR;
								//uint32_t reset_vector = *(__IO uint32_t*)(APPLICATION_2_ADDR + 4);
								//printf("\r\nApp Stack Ptr: 0x%08X, Reset Vector: 0x%08X\r\n", stack_pointer, reset_vector);
								//uint32_t stored_flag = *(__IO uint32_t*)FLAG_ADDR;
								//Log(0,"Stored boot flag: 0x%08X %s\r\n", stored_flag, stored_flag == STARTUP_UPDATE ? "(SUCCESS)" : "(FAIL)");
								/*TFT_SET_ADD(OTA_PROGRESS_X, OTA_TEXT_Y, 
                            OTA_PROGRESS_X + OTA_PROGRESS_WIDTH, 
                            OTA_TEXT_Y + 16);
                Draw_ASCII_Char(OTA_PROGRESS_X, OTA_TEXT_Y, 'O', OTA_TEXT_COLOR);
                Draw_ASCII_Char(OTA_PROGRESS_X+8, OTA_TEXT_Y, 'T', OTA_TEXT_COLOR);
                Draw_ASCII_Char(OTA_PROGRESS_X+16, OTA_TEXT_Y, 'A', OTA_TEXT_COLOR);
                Draw_ASCII_Char(OTA_PROGRESS_X+24, OTA_TEXT_Y, ' ', OTA_TEXT_COLOR);
                Draw_ASCII_Char(OTA_PROGRESS_X+32, OTA_TEXT_Y, 'D', OTA_TEXT_COLOR);
                Draw_ASCII_Char(OTA_PROGRESS_X+40, OTA_TEXT_Y, 'o', OTA_TEXT_COLOR);
                Draw_ASCII_Char(OTA_PROGRESS_X+48, OTA_TEXT_Y, 'n', OTA_TEXT_COLOR);
                Draw_ASCII_Char(OTA_PROGRESS_X+56, OTA_TEXT_Y, 'e', OTA_TEXT_COLOR);
                Draw_ASCII_Char(OTA_PROGRESS_X+64, OTA_TEXT_Y, '!', OTA_TEXT_COLOR);*/
								
								HAL_Delay(10000);  //延时确保上位机更新完升级日志再断电重启
                HAL_NVIC_SystemReset();           // 软件重启
						}
						continue;
				}
					
		}
}
void CAN_Send_pos_Ack(uint32_t can_id,int16_t pos, uint8_t ack_type, uint8_t frame_cnt) {
    // ack_type: 0x00=正确, 0xFF=错误
    uint32_t mailbox;
		int8_t pos1 = (int8_t)(pos & 0xFF);
	  int8_t pos2 = (int8_t)((pos >> 8) &0xFF);
    uint8_t ack[] = { ack_type, pos1, pos2, 0x00, 0x00, 0x00, frame_cnt, 0x00};
		ack[7] = xor_checksum(ack);
    txHeader.StdId = can_id;
    txHeader.IDE = CAN_ID_STD;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 8;
   // 等待邮箱有空位（可加超时保护）
    while (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0);
    HAL_StatusTypeDef ret = HAL_CAN_AddTxMessage(&hcan, &txHeader, ack, &mailbox);
    if (ret != HAL_OK) {
        printf("CAN ACK发送失败！\r\n");
    }
}
// -- CAN发送应答 --
void CAN_Send_Ack(uint32_t can_id,uint8_t ack_type, uint8_t frame_cnt) {
    // ack_type: 0x00=正确, 0xFF=错误
    uint32_t mailbox;
    uint8_t ack[] = { ack_type, 0x00, 0x00, 0x00, 0x00, 0x00, frame_cnt, 0x00};
		ack[7] = xor_checksum(ack);
    txHeader.StdId = can_id;
    txHeader.IDE = CAN_ID_STD;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 8;
   // 等待邮箱有空位（可加超时保护）
    while (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0);
    HAL_StatusTypeDef ret = HAL_CAN_AddTxMessage(&hcan, &txHeader, ack, &mailbox);
    if (ret != HAL_OK) {
        printf("CAN ACK发送失败！\r\n");
    }
}

void CAN_Send_Current_Ack(uint32_t can_id,int16_t cur, uint8_t ack_type, uint8_t frame_cnt){
		// ack_type: 0x00=正确, 0xFF=错误
    uint32_t mailbox;
		int8_t cur1 = (int8_t)(cur & 0xFF);
	  int8_t cur2 = (int8_t)((cur >> 8) &0xFF);
    uint8_t ack[] = { ack_type, cur1, cur2, 0x00, 0x00, 0x00, frame_cnt, 0x00};
		ack[7] = xor_checksum(ack);
    txHeader.StdId = can_id;
    txHeader.IDE = CAN_ID_STD;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.DLC = 8;
   // 等待邮箱有空位（可加超时保护）
    while (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0);
    HAL_StatusTypeDef ret = HAL_CAN_AddTxMessage(&hcan, &txHeader, ack, &mailbox);
    if (ret != HAL_OK) {
        printf("CAN ACK发送失败！\r\n");
    }
}
void can_queue_push(can_ring_queue_t *q, uint8_t *frame, uint32_t can_id) {
    int next = (q->tail + 1) % CAN_RX_QUEUE_SIZE;
    if (next == q->head) { // full
        q->head = (q->head + 1) % CAN_RX_QUEUE_SIZE;
        if (q->size > 0) q->size--;
    }
    memcpy(q->buf[q->tail].frame, frame, 8);
    q->buf[q->tail].can_id = can_id;
    q->tail = next;
    if (q->size < CAN_RX_QUEUE_SIZE) q->size++;
}

int can_queue_pop(can_ring_queue_t *q, can_frame_node_t *out) {
    if (q->head == q->tail) return 0;
    memcpy(out, &q->buf[q->head], sizeof(can_frame_node_t));
    q->head = (q->head + 1) % CAN_RX_QUEUE_SIZE;
    if (q->size > 0) q->size--;
    return 1;
}


// CAN接收中断回调
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {

	if (hcan->Instance == CAN1) {
		if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, rxdata) == HAL_OK) {
			if (rxHeader.StdId == LOG_TRIGGER_CAN_ID          
					&& rxHeader.DLC == LOG_TRIGGER_DLC            
					&& rxdata[0] == LOG_TRIGGER_DATA) {           
					g_trigger_log_send = 1;  // 设置触发标志
					return;
					
      }
			if (rxHeader.StdId == 0x109          
					&& rxHeader.DLC == 1            
					&& rxdata[0] == 0xAA) {          
					NVIC_SystemReset();
					return;
					
      }
			can_queue_push(&canQueue, rxdata, rxHeader.StdId);
			//printf("canQueue success\r\n");
		}
  }
}

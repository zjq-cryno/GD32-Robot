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
uint8_t xor_checksum(uint8_t *data, int len) {
    uint8_t res = 0;
    for (int i = 0; i < len; i++) {
        res ^= data[i];
    }
    return res;
}

void send_can_response_bytes(uint8_t *data, uint32_t can_id, CAN_HandleTypeDef *hcan) {
    uint8_t frame_data[CAN_FRAME_BYTES];
    uint8_t total_bytes = 12;
    uint8_t frames = (total_bytes + CAN_DATA_BYTES_PER_FRAME - 1) / CAN_DATA_BYTES_PER_FRAME;
    uint8_t frame_cnt = 1;
    uint8_t offset = 0;

    for (uint8_t i = 0; i < frames; i++) {
        memset(frame_data, 0, CAN_FRAME_BYTES);

        // 剩余字节数（本帧实际有效数据长度，除去frame计次和校验和）
        uint8_t bytes_this_frame = (total_bytes > CAN_DATA_BYTES_PER_FRAME) ? CAN_DATA_BYTES_PER_FRAME : total_bytes;
        frame_data[0] = bytes_this_frame;

        // 有效数据填充
        memcpy(&frame_data[1], &data[offset], bytes_this_frame);

        // 帧计次
        frame_data[6] = frame_cnt;

        // 校验和
        frame_data[7] = xor_checksum(frame_data,7);

        // 构造CAN发送结构体
        CAN_TxHeaderTypeDef txHeader;
        txHeader.StdId = can_id;
        txHeader.IDE = CAN_ID_STD;
        txHeader.RTR = CAN_RTR_DATA;
        txHeader.DLC = 8;
        txHeader.TransmitGlobalTime = DISABLE;

        uint32_t txMailbox;
        HAL_CAN_AddTxMessage(hcan, &txHeader, frame_data, &txMailbox);

        // 更新状态
        offset += bytes_this_frame;
        total_bytes -= bytes_this_frame;
        frame_cnt++;

        // 可适当延时，避免总线拥堵
        HAL_Delay(1);
    }
}

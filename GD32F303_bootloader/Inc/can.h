/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    can.h
  * @brief   This file contains all the function prototypes for
  *          the can.c file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CAN_H__
#define __CAN_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "string.h"
#include "stdio.h"


/* USER CODE BEGIN Includes */

	// 自定义协议帧结构
typedef struct {
  uint32_t id;        // CAN ID
  uint8_t  len;       // 数据长度 (0-8)
  uint8_t  data[8];   // CAN数据
  uint8_t  checksum;  // 校验和
} CAN_UART_Frame;

/* USER CODE END Includes */
extern CAN_HandleTypeDef hcan;

extern CAN_TxHeaderTypeDef txHeader;
extern CAN_RxHeaderTypeDef rxHeader;

extern uint8_t error_frame_num;
extern volatile uint8_t recv_error_flag;

#define CAN_RX_QUEUE_SIZE 100

typedef struct {
    uint8_t frame[8];
    uint32_t can_id;
} can_frame_node_t;

typedef struct {
    can_frame_node_t buf[CAN_RX_QUEUE_SIZE];
    volatile int head, tail, size;
} can_ring_queue_t;


// 新增高优先级队列
#define PRIORITY_QUEUE_SIZE 10
typedef struct {
    can_frame_node_t buf[PRIORITY_QUEUE_SIZE];
    int head;
    int tail;
    int size;
} can_priority_queue_t;


extern uint8_t txdata[8];
extern uint32_t TxID;
extern uint8_t TxLength;
extern uint8_t rxdata[8];
extern uint32_t RxID;
extern uint8_t RxLength;
extern uint8_t CAN1_Send_Msg(uint32_t id, uint8_t *msg, uint8_t len);
extern uint8_t CAN1_Recv_Msg(uint32_t *ID, uint8_t *buf, uint8_t *Length);

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_CAN_Init(void);
uint16_t CRC16_CCITT(const uint8_t *data, uint8_t length);
void SendPictureViaCAN(uint32_t baseID);
/* USER CODE BEGIN Prototypes */
HAL_StatusTypeDef CAN_SendMessage(uint32_t id, uint8_t *data, uint8_t dataLength);
void MX_CAN_Init(void);
/* USER CODE END Prototypes */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan);
uint8_t CAN1_Loopback(void);
uint8_t xor_checksum(uint8_t *data, int len);
void send_can_response_bytes(uint8_t *data, uint32_t can_id, CAN_HandleTypeDef *hcan);

#ifdef __cplusplus
}
#endif

#endif /* __CAN_H__ */


/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SPI1_DC_Pin GPIO_PIN_13
#define SPI1_DC_GPIO_Port GPIOC
#define SPI1_CS_Pin GPIO_PIN_4
#define SPI1_CS_GPIO_Port GPIOA
#define SPI2_CS_Pin GPIO_PIN_12
#define SPI2_CS_GPIO_Port GPIOB
#define SPI1_RST_Pin GPIO_PIN_8
#define SPI1_RST_GPIO_Port GPIOA
#define SPI1_MOSI_GPIO_Port GPIOA
#define SPI1_MOSI_Pin GPIO_PIN_7
#define SPI1_SCK_GPIO_Port	GPIOA
#define SPI1_SCK_Pin GPIO_PIN_5
/* USER CODE BEGIN Private defines */
extern uint16_t sq20,D,time_tim7,time_out,time;
extern int8_t uart5_enable,uart5_error;
extern uint8_t rx_frame[64];
extern uint8_t recv_end_flag_huart5;

extern int check_shutdown(void);


// 1. 全局变量定义（接收缓存、计数、终止符）
#define UART1_RX_BUF_SIZE 40  // 串口1接收缓存大小（可根据需求调整）
#define UART5_TX_BUF_SIZE 40  // 串口5发送缓存大小
#define END_CHAR '\n'         // 终止符（建议串口助手发送时加“换行”，与终止符匹配）


extern uint8_t uart1_rxbuf[UART1_RX_BUF_SIZE]; // 串口1接收缓存
extern uint8_t uart1_rx_cnt;                     // 串口1接收字节计数
extern uint8_t uart1_rx_flag;                    // 串口1接收完成标志（0：未完成，1：完成）
extern uint8_t uart5_txbuf[UART5_TX_BUF_SIZE]; // 串口5发送缓存
extern uint8_t uart5_rxbuf[UART5_TX_BUF_SIZE]; // 串口5接收缓存（若未使用可保留初始化）

extern void Log(uint8_t log_level,const char *fmt, ...);
extern void string_to_hex(uint8_t *dest, char *src, int *length);
extern int check_current(void);
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

#ifndef __HG25Q256_H
#define __HG25Q256_H

#include <stdint.h> // 包含标准整数类型定义
#include "main.h"
// 宏定义
#define PAGE_SIZE 256               // 每页大小
#define SECTOR_SIZE 4096            // 每个扇区的大小
#define BLOCK_SIZE 12800

#define IMAGE1_FLASH_ADDR 0x000000
#define IMAGE2_FLASH_ADDR 0x00C800  // 51200 = 0xC800

// 函数声明
void HG25Q256_Enable(void);
void HG25Q256_Disable(void);
void HG25Q256_Write_Enable(void);
uint8_t HG25Q256_ReadSR(void);
void HG25Q256_Wait_Busy(void);
void HG25Q256_Erase_Sector(uint32_t address);
void HG25Q256_Write_Page(const uint8_t* buffer, uint32_t address, uint16_t length);
void HG25Q256_Read_Data(uint8_t* buffer, uint32_t address, uint16_t length);
uint8_t hg25q256mw_read_id(void);
uint8_t HG25Q256_Send_Data_DMA(const uint8_t* data, uint32_t size, uint32_t address);
uint8_t HG25Q256_Read_Data_DMA(uint8_t* buffer, uint32_t size, uint32_t address);
extern volatile uint8_t dma_transfer_complete;
extern volatile uint8_t dma_transfer_complete_spi1;
extern uint32_t display_blocks;
void HG25Q256_Write_Multiple_Pages(uint32_t start_address, const uint8_t* data, uint32_t total_length) ;
void HG25Q256_Read_DMA(uint8_t* buffer, uint32_t start_address, uint32_t num_bytes);
void DisplayImageFromFlash(uint32_t flash_addr);
void DisplayAnimationFromFlash(void);

void WriteMultipleImagesToFlash(void);

extern uint8_t A[BLOCK_SIZE]; // 用于读取的缓冲区 A
extern uint8_t B[BLOCK_SIZE]; // 用于显示的缓冲区 B

//=============flash 测试============

extern void Flash_WriteRead_Test(void);
extern void Flash_DMA_SimpleTest(void);

//===================================
#endif // __HG25Q256_H




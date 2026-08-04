#ifndef TFT_H
#define TFT_H
#include "includes.h"

// OTA进度条显示参数（可根据屏显需求调整坐标）
#define OTA_PROGRESS_X      20    // 进度条左上角X坐标
#define OTA_PROGRESS_Y      100   // 进度条左上角Y坐标
#define OTA_PROGRESS_WIDTH  120   // 进度条总宽度（占满160屏大部分宽度）
#define OTA_PROGRESS_HEIGHT 10    // 进度条高度
#define OTA_TEXT_Y          120   // 进度百分比文字Y坐标（进度条下方）

// 进度条颜色定义
#define OTA_BG_COLOR        0x0000  // 进度条背景色（黑色）
#define OTA_FILL_COLOR      0x07E0  // 进度填充色（绿色，与电池绿色一致）
#define OTA_BORDER_COLOR    0xFFFF  // 进度条边框色（白色）
#define OTA_TEXT_COLOR      0xFFFF  // 文字颜色（白色）

void TFT_WaitDMA(void);
void SPI_SendByte(unsigned  char byte);
void TFT_SEND_CMD(unsigned char o_command);
void TFT_SEND_DATA(unsigned  char o_data);
void TFT_SEND_DATA_DMA(const uint8_t *data, size_t size);
void TFT_SET_ADD(unsigned short int X_START,unsigned short int Y_START,unsigned short int X_END,unsigned short int Y_END);
void TFT_clear(void);
void TFT_full(unsigned int color);
void TFT_init(void);
void Picture_Display(const unsigned char *ptr_pic);
void Picture_Display_DMA(const uint8_t *ptr_pic, size_t size);

void GC9D01_CS_LOW(void);

void GC9D01_CS_HIGH(void);

void GC9D01_DC_LOW(void);

void GC9D01_DC_HIGH(void);

void GC9D01_RESET_LOW(void);

void GC9D01_RESET_HIGH(void);

void GC9D01_WriteCommand(uint8_t cmd);

void GC9D01_WriteData(uint8_t data);

void GC9D01_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

void GC9D01_DrawPixel(uint16_t x, uint16_t y, uint16_t color);

void GC9D01_FillScreen(uint16_t color);

void GC9D01_Init(void);

#endif 





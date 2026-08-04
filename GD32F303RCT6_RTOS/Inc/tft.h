#ifndef TFT_H
#define TFT_H
#include "includes.h"

#define DISPLAY_ONE_PIC	0
#define DISPLAY_ANIMATION 1

extern uint8_t g_ota_last_progress;

void TFT_init(void) ;
void TFT_full(uint16_t color);
void Picture_Display(const unsigned char *ptr_pic, size_t size);
void TestPixelCoordinates(void);
void SetPixelColor(uint16_t x, uint16_t y, uint16_t color);
void TFT_full1(uint16_t color);
void Picture_Display1(const unsigned char *ptr_pic, size_t size);
void TFT_clear(void);
void TFT_draw_circle(uint16_t center_x, uint16_t center_y, uint16_t radius, uint16_t color);

void Show_BatteryBar_From_ADC_New(float adc_voltage);
void TFT_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void DrawRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color);
void FillRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color);
void DrawNewBattery(uint8_t percent);
void TFT_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void TFT_SEND_DATA_DMA(const uint8_t *data, size_t size);
void TFT_SET_ADD(uint16_t X_START, uint16_t Y_START, uint16_t X_END, uint16_t Y_END);
void TFT_Display_OTA_Progress(uint8_t progress);
void Draw_ASCII_Char(uint16_t x, uint16_t y, char c, uint16_t color);
#endif 





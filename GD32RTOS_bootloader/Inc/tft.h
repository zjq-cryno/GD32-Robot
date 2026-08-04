#ifndef TFT_H
#define TFT_H

void TFT_init(void) ;
void TFT_full(uint16_t color);
void Picture_Display(const unsigned char *ptr_pic, size_t size);
void TestPixelCoordinates(void);
void SetPixelColor(uint16_t x, uint16_t y, uint16_t color);
void TFT_full1(uint16_t color);
void Picture_Display1(const unsigned char *ptr_pic, size_t size);
void TFT_clear(void);
void TFT_draw_circle(uint16_t center_x, uint16_t center_y, uint16_t radius, uint16_t color);
void Fourpoint_Display(void);
#endif 





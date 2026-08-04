#include "tft.h"
#include "spi.h"
#include "hg25q256.h"

// 新电池图标参数定义
#define NEW_BAT_X          45      // 电池图标左上角X坐标
#define NEW_BAT_Y          60      // 电池图标左上角Y坐标
#define NEW_BAT_WIDTH      80      // 总主体宽度
#define NEW_BAT_HEIGHT     40      //电池主体高度
#define NEW_BAT_CORNER     6       //圆角半径
#define NEW_BAT_CAP_WIDTH  8       //电池正极宽度
#define NEW_BAT_CAP_HEIGHT 16      //电池正极高度
#define NEW_BAT_BORDER     4       //边框宽度
#define NEW_BAT_INSET      5       //内边距

// 颜色定义
#define COLOR_BORDER       0x8410  // 深灰色边框
#define COLOR_CAP          0x8410  // 电池正极颜色
#define COLOR_BG           0x0000  // 背景色(黑色)
#define COLOR_EMPTY        0x39E7  // 空电颜色(深灰)
#define COLOR_LOW          0xF800  // 低电量(红色)
#define COLOR_MID          0xFFE0  // 中等电量(黄色)
#define COLOR_HIGH         0x07E0  // 高电量(绿色)

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

// 全局变量（记录当前进度，避免重复绘制）
uint8_t g_ota_last_progress = 0;  // 上一次显示的进度，初始为0


extern SPI_HandleTypeDef hspi1;

void TFT_WaitDMA(void)
{
    while (!dma_transfer_complete_spi1);
    dma_transfer_complete_spi1 = 0; // 清零以便下次用
}


// SPI发送1字节
void SPI_SendByte(uint8_t byte)
{
   SPI1_CS_0;
    // 发送一个字节
	//HAL_SPI_Transmit(&hspi1, &byte, 1, HAL_MAX_DELAY);
	HAL_SPI_Transmit_DMA(&hspi1,&byte,1);
	/*
	if(HAL_SPI_Transmit_DMA(&hspi1,&byte,1)!=HAL_OK){        uint8_t error_byte = byte; // 记录有问题的字节
        HAL_UART_Transmit(&huart3, &error_byte, 1, HAL_MAX_DELAY);}
	while(HAL_SPI_GetState(&hspi1)!=HAL_SPI_STATE_READY)
	*/
   SPI1_CS_1;
}

// DMA方式发送数据
void TFT_SEND_DATA_DMA(const uint8_t *data, size_t size)
{
    SPI1_DC_1; // 设置 DC 引脚状态
    SPI1_CS_0;
		dma_transfer_complete_spi1 = 0;
    // 使用 DMA 发送整个数据
    HAL_SPI_Transmit_DMA(&hspi1, (uint8_t *)data, size);
	//while (HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY);
	//SPI1_CS_1;
}


void TFT_SEND_CMD(uint8_t command)
{
    SPI1_DC_0;
    SPI_SendByte(command);
    //SPI1_DC_1;
}

void TFT_SEND_DATA(uint8_t data)
{
    SPI1_DC_1;
    SPI_SendByte(data);
}


void TFT_SET_ADD(uint16_t X_START, uint16_t Y_START, uint16_t X_END, uint16_t Y_END)
{
    X_START += TFT_COLUMN_OFFSET;
    Y_START += TFT_LINE_OFFSET;
    X_END += TFT_COLUMN_OFFSET;
    Y_END += TFT_LINE_OFFSET;
		
		if (X_END >= TFT_COLUMN_NUMBER) X_END = TFT_COLUMN_NUMBER - 1;
    if (Y_END >= TFT_LINE_NUMBER) Y_END = TFT_LINE_NUMBER - 1;
	
    TFT_SEND_CMD(0x2A); // Column address set
    TFT_SEND_DATA(X_START >> 8); // start column
    TFT_SEND_DATA(X_START);
    TFT_SEND_DATA(X_END >> 8); // end column
    TFT_SEND_DATA(X_END);

    TFT_SEND_CMD(0x2B); // Row address set
    TFT_SEND_DATA(Y_START >> 8); // start row
    TFT_SEND_DATA(Y_START);
    TFT_SEND_DATA(Y_END >> 8); // end row
    TFT_SEND_DATA(Y_END);
    TFT_SEND_CMD(0x2C); // Memory write
}


void Picture_Display(const unsigned char *ptr_pic, size_t size)
	
{
    // 设置显示区域
    TFT_SET_ADD(0, 0, 159, 159);

    // 使用 DMA 发送数据，短时间内发送整个数组
    TFT_SEND_DATA_DMA(ptr_pic, size);
		TFT_WaitDMA();
}
void TFT_clear(void)
{
    uint16_t ROW, column;
    TFT_SET_ADD(0, 0, TFT_COLUMN_NUMBER - 1, TFT_LINE_NUMBER - 1);
    for (ROW = 0; ROW < TFT_LINE_NUMBER; ROW++) // ROW loop
    { 
        for (column = 0; column < TFT_COLUMN_NUMBER; column++) // column loop
        {
            TFT_SEND_DATA(0x00); // 颜色的高字节
            TFT_SEND_DATA(0x00); // 颜色的低字节
        }
    }
		// 全屏清空（白屏，DMA块写，效率高）
		/*static uint8_t line_buf[TFT_COLUMN_NUMBER * 2];
    for (int i = 0; i < TFT_COLUMN_NUMBER; i++) {
        line_buf[i * 2] = 0xFF;
        line_buf[i * 2 + 1] = 0xFF;
    }
    TFT_SET_ADD(0, 0, 159, 159);
    for (int row = 0; row < TFT_LINE_NUMBER; row++) {
        TFT_SEND_DATA_DMA(line_buf, TFT_COLUMN_NUMBER * 2);
        TFT_WaitDMA();
    }*/
		
}

void TFT_full(uint16_t color)
{
    uint16_t ROW, column;
    TFT_SET_ADD(0, 0, TFT_COLUMN_NUMBER - 1, TFT_LINE_NUMBER - 1);
    for (ROW = 0; ROW < TFT_LINE_NUMBER; ROW++) // ROW loop
    { 
        for (column = 0; column < TFT_COLUMN_NUMBER; column++) // column loop
        {
            TFT_SEND_DATA(color >> 8); // 颜色的高字节
            TFT_SEND_DATA(color);       // 颜色的低字节
        }
    }
		// 全屏填充某色，DMA块写
		/*static uint8_t line_buf[TFT_COLUMN_NUMBER * 2];
    for (int i = 0; i < TFT_COLUMN_NUMBER; i++) {
        line_buf[i * 2] = color >> 8;
        line_buf[i * 2 + 1] = color & 0xFF;
    }
    TFT_SET_ADD(0, 0, 159, 159);
    for (int row = 0; row < TFT_LINE_NUMBER; row++) {
        TFT_SEND_DATA_DMA(line_buf, TFT_COLUMN_NUMBER * 2);
        TFT_WaitDMA();
    }*/
}



void TFT_init(void)        ////GC9D01
{
	SPI1_RST_0;
	HAL_Delay(10);
	SPI1_RST_1;
	HAL_Delay(120);
	TFT_SEND_CMD(0xFE);
	TFT_SEND_CMD(0xEF); 
	TFT_SEND_CMD(0x80);
	TFT_SEND_DATA(0xFF);
		
	TFT_SEND_CMD(0x81);
	TFT_SEND_DATA(0xFF);

	TFT_SEND_CMD(0x82);
	TFT_SEND_DATA(0xFF);

	TFT_SEND_CMD(0x83);
	TFT_SEND_DATA(0xFF);

	TFT_SEND_CMD(0x84);		
	TFT_SEND_DATA(0xFF); 

	TFT_SEND_CMD(0x85);
	TFT_SEND_DATA(0xFF); 

	TFT_SEND_CMD(0x86);     
	TFT_SEND_DATA(0xFF); 

	TFT_SEND_CMD(0x87);   
	TFT_SEND_DATA(0xFF);

	TFT_SEND_CMD(0x88);       
	TFT_SEND_DATA(0xFF);

	TFT_SEND_CMD(0x89);     
	TFT_SEND_DATA(0xFF);

	TFT_SEND_CMD(0x8A);       
	TFT_SEND_DATA(0xFF);

	TFT_SEND_CMD(0x8B);        
	TFT_SEND_DATA(0xFF);

	TFT_SEND_CMD(0x8C);      
	TFT_SEND_DATA(0xFF);

	TFT_SEND_CMD(0x8D);     
	TFT_SEND_DATA(0xFF);

	TFT_SEND_CMD(0x8E);   
	TFT_SEND_DATA(0xFF); 

	TFT_SEND_CMD(0x8F);  
	TFT_SEND_DATA(0xFF); 

	TFT_SEND_CMD(0x3A);    
	TFT_SEND_DATA(0x05);

	TFT_SEND_CMD(0xEC);   
	TFT_SEND_DATA(0x01);

	TFT_SEND_CMD(0x74);	
	TFT_SEND_DATA(0x02);
	TFT_SEND_DATA(0x0E);
	TFT_SEND_DATA(0x00);
	TFT_SEND_DATA(0x00);
	TFT_SEND_DATA(0x00);
	TFT_SEND_DATA(0x00);
	TFT_SEND_DATA(0x00);

	TFT_SEND_CMD(0x98);    
	TFT_SEND_DATA(0x3E);
	TFT_SEND_CMD(0x99);    
	TFT_SEND_DATA(0x3E);

	TFT_SEND_CMD(0xB5);		
	TFT_SEND_DATA(0x0D);
	TFT_SEND_DATA(0x0D);

	TFT_SEND_CMD(0x60);	
	TFT_SEND_DATA(0x38);	
	TFT_SEND_DATA(0x0F);
	TFT_SEND_DATA(0x79);
	TFT_SEND_DATA(0x67);

	TFT_SEND_CMD(0x61);	
	TFT_SEND_DATA(0x11);
	TFT_SEND_DATA(0x79);
	TFT_SEND_DATA(0x67);

	TFT_SEND_CMD(0x64);		
	TFT_SEND_DATA(0x38);
	TFT_SEND_DATA(0x17);
	TFT_SEND_DATA(0x71);
	TFT_SEND_DATA(0x5F);
	TFT_SEND_DATA(0x79);
	TFT_SEND_DATA(0x67);

	TFT_SEND_CMD(0x65);		
	TFT_SEND_DATA(0x38);
	TFT_SEND_DATA(0x13);
	TFT_SEND_DATA(0x71);
	TFT_SEND_DATA(0x5B);
	TFT_SEND_DATA(0x79);
	TFT_SEND_DATA(0x67);

	TFT_SEND_CMD(0x6A);	
	TFT_SEND_DATA(0x00);
	TFT_SEND_DATA(0x00);

	TFT_SEND_CMD(0x6C);			
	TFT_SEND_DATA(0x22);
	TFT_SEND_DATA(0x02);
	TFT_SEND_DATA(0x22);
	TFT_SEND_DATA(0x02);
	TFT_SEND_DATA(0x22);
	TFT_SEND_DATA(0x22);
	TFT_SEND_DATA(0x50);


    TFT_SEND_CMD(0x6E);
    TFT_SEND_DATA(0x03);
    TFT_SEND_DATA(0x03);
    TFT_SEND_DATA(0x01);
    TFT_SEND_DATA(0x01);
    TFT_SEND_DATA(0x00);
    TFT_SEND_DATA(0x00);
    TFT_SEND_DATA(0x0F);
    TFT_SEND_DATA(0x0F); 
    TFT_SEND_DATA(0x0D);
    TFT_SEND_DATA(0x0D);
    TFT_SEND_DATA(0x0B);
    TFT_SEND_DATA(0x0B);
    TFT_SEND_DATA(0x09);
    TFT_SEND_DATA(0x09);
    TFT_SEND_DATA(0x00);
    TFT_SEND_DATA(0x00);
    TFT_SEND_DATA(0x00);
    TFT_SEND_DATA(0x00);
    TFT_SEND_DATA(0x0A);
    TFT_SEND_DATA(0x0A);
    TFT_SEND_DATA(0x0c);
    TFT_SEND_DATA(0x0c);
    TFT_SEND_DATA(0x0e);
    TFT_SEND_DATA(0x0e);
    TFT_SEND_DATA(0x10);
    TFT_SEND_DATA(0x10);
    TFT_SEND_DATA(0x00);
    TFT_SEND_DATA(0x00);
    TFT_SEND_DATA(0x02);
    TFT_SEND_DATA(0x02);
    TFT_SEND_DATA(0x04);
    TFT_SEND_DATA(0x04);

    TFT_SEND_CMD(0xBF);
	TFT_SEND_DATA(0x01);

	TFT_SEND_CMD(0xF9);		
	TFT_SEND_DATA(0x40);
	TFT_SEND_CMD(0x9b);	
	TFT_SEND_DATA(0x3b);
	
	
	TFT_SEND_CMD(0x93);
	TFT_SEND_DATA(0x33);
	TFT_SEND_DATA(0x7f);
	TFT_SEND_DATA(0x00);

	TFT_SEND_CMD(0x7E);   
	TFT_SEND_DATA(0x30);

	TFT_SEND_CMD(0x70);
	TFT_SEND_DATA(0x0d);
	TFT_SEND_DATA(0x02);
	TFT_SEND_DATA(0x08);
	TFT_SEND_DATA(0x0d);
	TFT_SEND_DATA(0x02);
	TFT_SEND_DATA(0x08);

	TFT_SEND_CMD(0x71);
	TFT_SEND_DATA(0x0d);
	TFT_SEND_DATA(0x02);
	TFT_SEND_DATA(0x08);

	TFT_SEND_CMD(0x91);
	TFT_SEND_DATA(0x0E);
	TFT_SEND_DATA(0x09);

	TFT_SEND_CMD(0xc3);
	TFT_SEND_DATA(0x18);
	TFT_SEND_CMD(0xc4);
	TFT_SEND_DATA(0x18);
	TFT_SEND_CMD(0xc9);
	TFT_SEND_DATA(0x3c);

	TFT_SEND_CMD(0xf0);
	TFT_SEND_DATA(0x13);
	TFT_SEND_DATA(0x15);
	TFT_SEND_DATA(0x04);
	TFT_SEND_DATA(0x05);
	TFT_SEND_DATA(0x01);
	TFT_SEND_DATA(0x38);

	TFT_SEND_CMD(0xf2);
	TFT_SEND_DATA(0x13);
	TFT_SEND_DATA(0x15);
	TFT_SEND_DATA(0x04);
	TFT_SEND_DATA(0x05);
	TFT_SEND_DATA(0x01);
	TFT_SEND_DATA(0x34);

	TFT_SEND_CMD(0xf1);
	TFT_SEND_DATA(0x4b);
	TFT_SEND_DATA(0xb8);
	TFT_SEND_DATA(0x7b);
	TFT_SEND_DATA(0x34);
	TFT_SEND_DATA(0x35);
	TFT_SEND_DATA(0xef);

	TFT_SEND_CMD(0xf3);
	TFT_SEND_DATA(0x47);
	TFT_SEND_DATA(0xb4);
	TFT_SEND_DATA(0x72);
	TFT_SEND_DATA(0x34);
	TFT_SEND_DATA(0x35);
	TFT_SEND_DATA(0xda);

	TFT_SEND_CMD(0x36);			
	TFT_SEND_DATA(0x00);
	TFT_SEND_CMD(0xB4);		
	TFT_SEND_DATA(0x00);
	TFT_SEND_DATA(0x00);
	TFT_SEND_CMD(0x34);
	TFT_SEND_CMD(0x11);

	HAL_Delay(120);
	TFT_SEND_CMD(0x29); //Display on // 开显示
  }



void SetPixelColor(uint16_t x, uint16_t y, uint16_t color) {
    // 绘制 2x2 的正方形
    TFT_SET_ADD(x, y, x + PIXEL_MULTIPLIER - 1, y + PIXEL_MULTIPLIER - 1);  // 设置显示区域为2x2

    // 两个字节循环表示颜色
    for (uint16_t i = 0; i < PIXEL_MULTIPLIER * PIXEL_MULTIPLIER; i++) {
        TFT_SEND_DATA(color >> 8); // 发送颜色高字节
        TFT_SEND_DATA(color);       // 发送颜色低字节
    }
}

void TestPixelCoordinates() {
    // 将屏幕清空
    TFT_clear();

    // 测试不同的坐标
    // 左上角 (0,0)
    SetPixelColor(80, 80, RED);

}


void Picture_Display1(const unsigned char *ptr_pic, size_t size) {
    
		TFT_SET_ADD(0, 0, TFT_COLUMN_NUMBER - 1, TFT_LINE_NUMBER - 1);
    // 重置DMA完成标志位，启动DMA传输
    dma_transfer_complete_spi1 = 0;
    TFT_SEND_DATA_DMA(ptr_pic, size);
  
}

void TFT_draw_circle(uint16_t center_x, uint16_t center_y, uint16_t radius, uint16_t color) {
    int x, y;
    int radius_squared = radius * radius; // 预计算半径的平方提高效率

    // 确保坐标在有效范围内
    for (y = -radius; y <= radius; y++) {
        for (x = -radius; x <= radius; x++) {
            if (x * x + y * y <= radius_squared) { // 判断点是否在圆内
                // 计算实际的屏幕坐标
                uint16_t pixel_x = center_x + x;
                uint16_t pixel_y = center_y + y;

                // 限制坐标在屏幕范围内
                if (pixel_x < TFT_COLUMN_NUMBER && pixel_y < TFT_LINE_NUMBER) {
                    // 使用 DMA 方式传输数据
                    TFT_SET_ADD(pixel_x, pixel_y, pixel_x, pixel_y); 
                    TFT_SEND_DATA(color >> 8); // 颜色的高字节
                    TFT_SEND_DATA(color);       // 颜色的低字节
                }
            }
        }
    }
}

// 填充矩形区域
void TFT_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (x >= TFT_COLUMN_NUMBER) return;
    if (y >= TFT_LINE_NUMBER) return;
    if ((x + w) > TFT_COLUMN_NUMBER) w = TFT_COLUMN_NUMBER - x;
    if ((y + h) > TFT_LINE_NUMBER) h = TFT_LINE_NUMBER - y;

    TFT_SET_ADD(x, y, x + w - 1, y + h - 1);

    static uint8_t buf[TFT_COLUMN_NUMBER * 2];
    for (uint16_t i = 0; i < w; i++) {
        buf[i * 2] = color >> 8;
        buf[i * 2 + 1] = color & 0xFF;
    }
    for (uint16_t row = 0; row < h; row++) {
        TFT_SEND_DATA_DMA(buf, w * 2);
        TFT_WaitDMA();
    }
}


// 绘制圆角矩形
void DrawRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color) {
    // 确保圆角半径不超过矩形尺寸的一半
    if (r > w/2) r = w/2;
    if (r > h/2) r = h/2;
    
    // 绘制四个角
    TFT_draw_circle(x + r, y + r, r, color);            // 左上角
    TFT_draw_circle(x + w - r - 1, y + r, r, color);    // 右上角
    TFT_draw_circle(x + r, y + h - r - 1, r, color);   // 左下角
    TFT_draw_circle(x + w - r - 1, y + h - r - 1, r, color); // 右下角
    
    // 绘制四条边
    TFT_FillRect(x + r, y, w - 2*r, r + 1, color);      // 上边
    TFT_FillRect(x + r, y + h - r - 1, w - 2*r, r + 1, color); // 下边
    TFT_FillRect(x, y + r, r + 1, h - 2*r, color);      // 左边
    TFT_FillRect(x + w - r - 1, y + r, r + 1, h - 2*r, color); // 右边
    
    // 填充中间区域
    TFT_FillRect(x + r + 1, y + r + 1, w - 2*r - 2, h - 2*r - 2, color);
}

// 绘制带圆角的填充矩形
void FillRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color) {
    // 确保圆角半径不超过矩形尺寸的一半
    if (r > w/2) r = w/2;
    if (r > h/2) r = h/2;
    
    // 绘制四个角
    TFT_draw_circle(x + r, y + r, r, color);            // 左上角
    TFT_draw_circle(x + w - r - 1, y + r, r, color);    // 右上角
    TFT_draw_circle(x + r, y + h - r - 1, r, color);   // 左下角
    TFT_draw_circle(x + w - r - 1, y + h - r - 1, r, color); // 右下角
    
    // 填充中间区域
    TFT_FillRect(x + r, y, w - 2*r, h, color);          // 水平填充
    TFT_FillRect(x, y + r, w, h - 2*r, color);          // 垂直填充
}

// 绘制新电池图标
void DrawNewBattery(uint8_t percent) {
    // 计算电量条宽度
    uint16_t bar_width = (NEW_BAT_WIDTH - 2*NEW_BAT_INSET - 2*NEW_BAT_BORDER) * percent / 100;
    
    // 1. 绘制电池主体边框
    DrawRoundRect(NEW_BAT_X, NEW_BAT_Y, NEW_BAT_WIDTH, NEW_BAT_HEIGHT, NEW_BAT_CORNER, COLOR_BORDER);
    
    // 2. 绘制电池正极
    uint16_t cap_x = NEW_BAT_X + NEW_BAT_WIDTH;
    uint16_t cap_y = NEW_BAT_Y + (NEW_BAT_HEIGHT - NEW_BAT_CAP_HEIGHT)/2;
    TFT_FillRect(cap_x, cap_y, NEW_BAT_CAP_WIDTH, NEW_BAT_CAP_HEIGHT, COLOR_CAP);
    
    // 3. 绘制电池内部背景
    //FillRoundRect(NEW_BAT_X + NEW_BAT_BORDER, NEW_BAT_Y + NEW_BAT_BORDER, 
                 //NEW_BAT_WIDTH - 2*NEW_BAT_BORDER, NEW_BAT_HEIGHT - 2*NEW_BAT_BORDER, 
                 //NEW_BAT_CORNER - 1, COLOR_BG);
    
    // 4. 绘制电量条
    if (bar_width > 0) {
        // 根据电量选择颜色
        uint16_t bar_color;
        if (percent <= 20) {
            bar_color = COLOR_LOW;    // 低电量红色
        } else if (percent <= 50) {
            bar_color = COLOR_MID;    // 中等电量黄色
        } else {
            bar_color = COLOR_HIGH;   // 高电量绿色
        }
        
        // 绘制电量条
        FillRoundRect(NEW_BAT_X + NEW_BAT_BORDER + NEW_BAT_INSET, 
                     NEW_BAT_Y + NEW_BAT_BORDER + NEW_BAT_INSET, 
                     bar_width, 
                     NEW_BAT_HEIGHT - 2*NEW_BAT_BORDER - 2*NEW_BAT_INSET, 
                     NEW_BAT_CORNER - 2, bar_color);
    }
    
    // 5. 绘制电量百分比文本
    char text[5];
    snprintf(text, sizeof(text), "%d%%", percent);
    
    // 计算文本位置（居中显示）
    uint16_t text_x = NEW_BAT_X + NEW_BAT_WIDTH/2 - 10; // 假设每个字符宽度约5像素
    uint16_t text_y = NEW_BAT_Y + NEW_BAT_HEIGHT/2 - 5; // 假设字符高度约10像素
    
    // 这里需要添加文本显示函数，假设您有一个简单的文本显示函数
    // DisplayText(text_x, text_y, text, COLOR_BORDER, COLOR_BG);
}

// 更新电池显示函数
void Show_BatteryBar_From_ADC_New(float adc_voltage) {
    // 电池电压范围：3.3V (空) - 4.2V (满)
    float full_voltage = 4.2f;
    float empty_voltage = 3.3f;
    
    // 计算电量百分比
    float percent = (adc_voltage - empty_voltage) / (full_voltage - empty_voltage) * 100;
    if (percent > 100) percent = 100;
    if (percent < 0) percent = 0;
    
    // 绘制新电池图标
    DrawNewBattery((uint8_t)percent);
}
/**
 * @brief 绘制OTA升级进度（进度条+百分比文字）
 * @param progress: 当前升级进度（0~100）
 */
void TFT_Display_OTA_Progress(uint8_t progress) {
    // 1. 边界处理：进度值限定在0~100
    if (progress > 100) progress = 100;
    // 优化：进度无变化时不重复绘制，减少SPI开销
    if (progress == g_ota_last_progress) return;
    g_ota_last_progress = progress;

    // 2. 绘制进度条背景（黑色填充+白色边框）
    // 2.1 填充背景（黑色矩形）
    TFT_SET_ADD(OTA_PROGRESS_X, OTA_PROGRESS_Y, 
                OTA_PROGRESS_X + OTA_PROGRESS_WIDTH, 
                OTA_PROGRESS_Y + OTA_PROGRESS_HEIGHT);
    for (uint16_t y = 0; y < OTA_PROGRESS_HEIGHT; y++) {
        for (uint16_t x = 0; x < OTA_PROGRESS_WIDTH; x++) {
            TFT_SEND_DATA(OTA_BG_COLOR >> 8);  // 颜色高字节
            TFT_SEND_DATA(OTA_BG_COLOR & 0xFF); // 颜色低字节
        }
    }
    // 2.2 绘制边框（白色矩形框，仅描边）
    // 上边框（Y=OTA_PROGRESS_Y，X从左到右）
    TFT_SET_ADD(OTA_PROGRESS_X, OTA_PROGRESS_Y, 
                OTA_PROGRESS_X + OTA_PROGRESS_WIDTH, 
                OTA_PROGRESS_Y);
    for (uint16_t x = 0; x < OTA_PROGRESS_WIDTH; x++) {
        TFT_SEND_DATA(OTA_BORDER_COLOR >> 8);
        TFT_SEND_DATA(OTA_BORDER_COLOR & 0xFF);
    }
    // 下边框（Y=OTA_PROGRESS_Y+HEIGHT，X从左到右）
    TFT_SET_ADD(OTA_PROGRESS_X, OTA_PROGRESS_Y + OTA_PROGRESS_HEIGHT, 
                OTA_PROGRESS_X + OTA_PROGRESS_WIDTH, 
                OTA_PROGRESS_Y + OTA_PROGRESS_HEIGHT);
    for (uint16_t x = 0; x < OTA_PROGRESS_WIDTH; x++) {
        TFT_SEND_DATA(OTA_BORDER_COLOR >> 8);
        TFT_SEND_DATA(OTA_BORDER_COLOR & 0xFF);
    }
    // 左边框（X=OTA_PROGRESS_X，Y从上到下）
    TFT_SET_ADD(OTA_PROGRESS_X, OTA_PROGRESS_Y, 
                OTA_PROGRESS_X, 
                OTA_PROGRESS_Y + OTA_PROGRESS_HEIGHT);
    for (uint16_t y = 0; y < OTA_PROGRESS_HEIGHT; y++) {
        TFT_SEND_DATA(OTA_BORDER_COLOR >> 8);
        TFT_SEND_DATA(OTA_BORDER_COLOR & 0xFF);
    }
    // 右边框（X=OTA_PROGRESS_X+WIDTH，Y从上到下）
    TFT_SET_ADD(OTA_PROGRESS_X + OTA_PROGRESS_WIDTH, OTA_PROGRESS_Y, 
                OTA_PROGRESS_X + OTA_PROGRESS_WIDTH, 
                OTA_PROGRESS_Y + OTA_PROGRESS_HEIGHT);
    for (uint16_t y = 0; y < OTA_PROGRESS_HEIGHT; y++) {
        TFT_SEND_DATA(OTA_BORDER_COLOR >> 8);
        TFT_SEND_DATA(OTA_BORDER_COLOR & 0xFF);
    }

    // 3. 绘制进度填充块（绿色，宽度=总宽度*进度/100）
    uint16_t fill_width = (OTA_PROGRESS_WIDTH * progress) / 100;
    if (fill_width > 0) {  // 进度>0时才绘制
        TFT_SET_ADD(OTA_PROGRESS_X + 1, OTA_PROGRESS_Y + 1,  // 偏移1像素，避免覆盖边框
                    OTA_PROGRESS_X + fill_width - 1, 
                    OTA_PROGRESS_Y + OTA_PROGRESS_HEIGHT - 1);
        for (uint16_t y = 0; y < OTA_PROGRESS_HEIGHT - 2; y++) {  // 减2：避开上下边框
            for (uint16_t x = 0; x < fill_width - 2; x++) {       // 减2：避开左右边框
                TFT_SEND_DATA(OTA_FILL_COLOR >> 8);
                TFT_SEND_DATA(OTA_FILL_COLOR & 0xFF);
            }
        }
    }

    // 4. 显示进度百分比文字（如"OTA: 50%"）
    char text[20];
    sprintf(text, "OTA: %d%%", progress);  // 格式化文字
    // 设置文字显示位置（X居中，Y=OTA_TEXT_Y）
    uint16_t text_x = OTA_PROGRESS_X + (OTA_PROGRESS_WIDTH - (strlen(text) * 8)) / 2;  // 假设每个字符8像素宽
    TFT_SET_ADD(text_x, OTA_TEXT_Y, text_x + (strlen(text) * 8), OTA_TEXT_Y + 16);  // 文字区域（高度16像素）
    // 注：若原代码无字符显示函数，需补充简单的ASCII字符绘制逻辑（示例如下）
    // （若已有TFT_Draw_Char函数，直接调用TFT_Draw_String(text_x, OTA_TEXT_Y, text, OTA_TEXT_COLOR)即可）
    for (int i = 0; text[i] != '\0'; i++) {
        Draw_ASCII_Char(text_x + i*8, OTA_TEXT_Y, text[i], OTA_TEXT_COLOR);
    }
}

/**
 * @brief 辅助函数：绘制单个ASCII字符（8x16点阵，适配160x160屏）
 * @param x: 字符左上角X坐标
 * @param y: 字符左上角Y坐标
 * @param c: 要绘制的ASCII字符（0x20~0x7E）
 * @param color: 字符颜色
 */
void Draw_ASCII_Char(uint16_t x, uint16_t y, char c, uint16_t color) {
    if (c < 0x20 || c > 0x7E) return;  // 只支持可见ASCII字符
    const uint8_t ascii_8x16[96][16] = {  // 8x16 ASCII点阵表（需自行补充完整，此处为示例结构）
        {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 空格(0x20)
        {0x00,0x00,0x7C,0x12,0x11,0x12,0x7C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // '0'(0x30)
        // ... 其他字符点阵（需补充完整，可从网上获取8x16 ASCII点阵数据）
    };
    const uint8_t* char_dot = ascii_8x16[c - 0x20];  // 获取当前字符的点阵数据

    // 逐行绘制字符（共16行，每行8列）
    for (uint8_t row = 0; row < 16; row++) {
        uint8_t dot = char_dot[row];
        TFT_SET_ADD(x, y + row, x + 7, y + row);  // 绘制当前行（8个像素）
        for (uint8_t col = 0; col < 8; col++) {
            if (dot & (0x80 >> col)) {  // 点阵为1时绘制颜色，0时不绘制（透明）
                TFT_SEND_DATA(color >> 8);
                TFT_SEND_DATA(color & 0xFF);
            } else {
                // 透明背景：绘制进度条背景色（避免覆盖原有内容）
                TFT_SEND_DATA(OTA_BG_COLOR >> 8);
                TFT_SEND_DATA(OTA_BG_COLOR & 0xFF);
            }
        }
    }
}

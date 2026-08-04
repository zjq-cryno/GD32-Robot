#include "includes.h"
#include "app.h"



void SPI_SendByte(uint8_t byte)
{
   SPI1_CS_0;
    // 发送一个字节
	HAL_SPI_Transmit(&hspi1, &byte, 1, HAL_MAX_DELAY);
	//HAL_SPI_Transmit_DMA(&hspi1,&byte,1);
	/*
	if(HAL_SPI_Transmit_DMA(&hspi1,&byte,1)!=HAL_OK){        uint8_t error_byte = byte; // 记录有问题的字节
        HAL_UART_Transmit(&huart3, &error_byte, 1, HAL_MAX_DELAY);}
	while(HAL_SPI_GetState(&hspi1)!=HAL_SPI_STATE_READY)
	*/
   SPI1_CS_1;
}

void TFT_SEND_DATA_DMA(const uint8_t *data, size_t size)
{
    SPI1_DC_1; // 设置 DC 引脚状态
    SPI1_CS_0;
    // 使用 DMA 发送整个数据
    HAL_SPI_Transmit_DMA(&hspi1, (uint8_t *)data, size);
	//while (HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY);
	//SPI1_CS_1;
}


void TFT_SEND_CMD(uint8_t command)
{
    SPI1_DC_0;
    SPI_SendByte(command);
    SPI1_DC_1;
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
    TFT_SET_ADD(0, 0, 160, 160);

    // 使用 DMA 发送数据，短时间内发送整个数组
    TFT_SEND_DATA_DMA(ptr_pic, size);
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
}


void TFT_full1(uint16_t color)
{
    uint16_t ROW, column;
	TFT_clear();
    TFT_SET_ADD(70, 70, 90, 90);
    for (ROW = 0; ROW < TFT_LINE_NUMBER; ROW++) // ROW loop
    { 
        for (column = 0; column < TFT_COLUMN_NUMBER; column++) // column loop
        {
            TFT_SEND_DATA(color >> 8); // 颜色的高字节
            TFT_SEND_DATA(color);       // 颜色的低字节
        }
    }
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
    uint16_t pixels_per_block = size / 2; // 假设RGB565格式，每个像素2字节

    // 计算每块显示的实际行数
    uint16_t rows_full = pixels_per_block / TFT_COLUMN_NUMBER;
    
    // 计算起始和结束行
    uint16_t y_start = (display_blocks * rows_full) % TFT_LINE_NUMBER;  // 确保y_start不超出160
    uint16_t y_end = y_start + rows_full - 1; // 计算结束行

    // 防止y_end超出显示范围
    if (y_end >= TFT_LINE_NUMBER) {
        y_end = TFT_LINE_NUMBER - 1; // 限制y_end
    }

    // 设置显示区域
    uint16_t x_start = 0; // 从左到右开始
    TFT_SET_ADD(x_start, y_start, x_start + (TFT_COLUMN_NUMBER - 1), y_end); // 设置为160的宽度

    // 使用 DMA 传输数据
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

void Fourpoint_Display()
{
		ReadAndDisplayImage(0x0000d000*2,3);	
		ReadAndDisplayImage(0x0000d000*3,4);
		ReadAndDisplayImage(0x0000d000*4,5);
		ReadAndDisplayImage(0x0000d000*5,6);
		ReadAndDisplayImage(0x0000d000*6,7);
		ReadAndDisplayImage(0x0000d000*7,8);
		ReadAndDisplayImage(0x0000d000*8,9);
		ReadAndDisplayImage(0x0000d000*9,10);
		ReadAndDisplayImage(0x0000d000*10,11);
		ReadAndDisplayImage(0x0000d000*11,12);
		ReadAndDisplayImage(0x0000d000*12,13);
		ReadAndDisplayImage(0x0000d000*13,14);
		ReadAndDisplayImage(0x0000d000*14,15);
		ReadAndDisplayImage(0x0000d000*15,16);
		ReadAndDisplayImage(0x0000d000*16,17);
		ReadAndDisplayImage(0x0000d000*17,18);
		ReadAndDisplayImage(0x0000d000*18,19);
		ReadAndDisplayImage(0x0000d000*19,20);
		ReadAndDisplayImage(0x0000d000*20,21);
		ReadAndDisplayImage(0x0000d000*21,22);
		ReadAndDisplayImage(0x0000d000*22,23);
		ReadAndDisplayImage(0x0000d000*23,24);
		ReadAndDisplayImage(0x0000d000*24,25);
		ReadAndDisplayImage(0x0000d000*25,26);
		ReadAndDisplayImage(0x0000d000*26,27);
		ReadAndDisplayImage(0x0000d000*27,28);
		ReadAndDisplayImage(0x0000d000*28,29);
		ReadAndDisplayImage(0x0000d000*29,30);
		ReadAndDisplayImage(0x0000d000*30,31);
		ReadAndDisplayImage(0x0000d000*31,32);
	
	
}
//写入数据
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*3,gImage_1,51200);
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*4,gImage_2,51200);
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*5,gImage_3,51200);
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*6,gImage_4,51200);
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*7,gImage_5,51200);
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*8,gImage_6,51200);
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*9,gImage_7,51200);
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*10,gImage_8,51200);
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*11,gImage_9,51200);
   //HG25Q256_Write_Multiple_Pages(0x0000d000*12,gImage_10,51200);
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*13,gImage_11,51200);
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*14,gImage_12,51200);
	 
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*15,gImage_13,51200);
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*16,gImage_14,51200);
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*17,gImage_15,51200);
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*18,gImage_16,51200);
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*19,gImage_17,51200);
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*20,gImage_18,51200);
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*21,gImage_19,51200);
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*22,gImage_20,51200);
	 
	 
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*23,gImage_21,51200);
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*24,gImage_22,51200);
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*25,gImage_23,51200);
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*26,gImage_24,51200);
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*27,gImage_25,51200);
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*28,gImage_26,51200);
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*29,gImage_27,51200);
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*30,gImage_28,51200);
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*31,gImage_29,51200);
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*32,gImage_30,51200);
	 //HG25Q256_Write_Multiple_Pages(0x0000d000*33,gImage_31,51200);
	 
	 
	 
	 
	 
	 


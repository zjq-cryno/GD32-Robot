#include "includes.h"
#include "app.h"

void TFT_WaitDMA(void)
{
    while (!dma_transfer_complete_spi1);
    dma_transfer_complete_spi1 = 0; // 清零以便下次用
}

void SPI_SendByte(unsigned  char byte)				//向液晶屏写一个8位数据
{
  
  unsigned char counter;
	SPI1_CS_0;
   
  for(counter=0;counter<8;counter++)
  { 
    SPI1_SCK_0;	  
    if((byte&0x80)==0)
    {
      SPI1_MOSI_0;
    }
    else SPI1_MOSI_1;
    byte=byte<<1;	
    SPI1_SCK_1;			
  }
  
  
		SPI1_CS_1;				//上拉WR确定数据输入
	SPI1_SCK_0;
}

// SPI发送1字节
void SPI_SendByte_DMA(uint8_t byte)
{
   SPI1_CS_0;
    // 发送一个字节
	//HAL_SPI_Transmit(&hspi1, &byte, 1, HAL_MAX_DELAY);
	HAL_SPI_Transmit_DMA(&hspi1,&byte,1);
	
   SPI1_CS_1;
}
// DMA方式发送数据
void TFT_SEND_DATA_DMA(const unsigned char *data, size_t size)
{
    SPI1_DC_1; // 设置 DC 引脚状态
    SPI1_CS_0;
		dma_transfer_complete_spi1 = 0;
    // 使用 DMA 发送整个数据
    HAL_SPI_Transmit_DMA(&hspi1, (uint8_t *)data, size);
	//while (HAL_SPI_GetState(&hspi1) != HAL_SPI_STATE_READY);
	//SPI1_CS_1;
}
void TFT_SEND_CMD(unsigned char o_command)
  {
    SPI1_DC_0;
    //SPI_WR_0;
    SPI_SendByte(o_command);
   // SPI_WR_1;
   
    //SPI_DC_1;
  }
  //向液晶屏写一个8位数据
void TFT_SEND_DATA(unsigned  char o_data)
  { 
    SPI1_DC_1;
   // SPI_WR_0;
    SPI_SendByte(o_data);
   // SPI_WR_1;
    
   }

void TFT_SET_ADD(unsigned short int X_START,unsigned short int Y_START,unsigned short int X_END,unsigned short int Y_END)
{
	X_START += TFT_COLUMN_OFFSET;
	Y_START += TFT_LINE_OFFSET;
	X_END += TFT_COLUMN_OFFSET;
	Y_END += TFT_LINE_OFFSET;
	TFT_SEND_CMD(0x2a); 		//Column address set
	TFT_SEND_DATA(X_START>>8); 		//start column
	TFT_SEND_DATA(X_START); 
	TFT_SEND_DATA(X_END>>8);		//end column
	TFT_SEND_DATA(X_END);

	TFT_SEND_CMD(0x2b); 		//Row address set
	TFT_SEND_DATA(Y_START>>8); 		//start row
	TFT_SEND_DATA(Y_START); 
	TFT_SEND_DATA(Y_END>>8);		//end row
	TFT_SEND_DATA(Y_END);
	TFT_SEND_CMD(0x2C);			//Memory write
}
void TFT_clear(void)
{
    unsigned int ROW,column;
	TFT_SET_ADD(0,0,TFT_COLUMN_NUMBER - 1,TFT_LINE_NUMBER - 1);
    for(ROW=0;ROW<TFT_LINE_NUMBER;ROW++)             //ROW loop
     { 
    
         for(column=0;column<TFT_COLUMN_NUMBER;column++)  //column loop
         {
              
			TFT_SEND_DATA(0x00);
			TFT_SEND_DATA(0x00);
         }
     }
}
void TFT_full(unsigned int color)
  {
    unsigned int ROW,column;
    TFT_SET_ADD(0,0,TFT_COLUMN_NUMBER - 1,TFT_LINE_NUMBER - 1);
    for(ROW=0;ROW<TFT_LINE_NUMBER;ROW++)             //ROW loop
      { 
    
  for(column=0;column<TFT_COLUMN_NUMBER ;column++) //column loop
          {

			TFT_SEND_DATA(color>>8);
			  TFT_SEND_DATA(color);
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
	TFT_SEND_DATA(0x38);
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
	TFT_SEND_DATA(0x0f);
	TFT_SEND_DATA(0x0f);
	TFT_SEND_DATA(0x0d);
	TFT_SEND_DATA(0x0d);
	TFT_SEND_DATA(0x0b);
	TFT_SEND_DATA(0x0b);
	TFT_SEND_DATA(0x09);
	TFT_SEND_DATA(0x09);
	TFT_SEND_DATA(0x00);
	TFT_SEND_DATA(0x00);
	TFT_SEND_DATA(0x00);
	TFT_SEND_DATA(0x00);
	TFT_SEND_DATA(0x0a);//
	TFT_SEND_DATA(0x0a);//
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

	TFT_SEND_CMD(0xbf);			
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
void Picture_Display(const unsigned char *ptr_pic)
{
    unsigned long  number;
	 TFT_SET_ADD(0,0,159,159);
	for(number=0;number<51200;number++)	
          {
//            data=*ptr_pic++;
//            data=~data;
              TFT_SEND_DATA(*ptr_pic++);
	
          }
  }

void Picture_Display_DMA(const uint8_t *ptr_pic, size_t size)
{
  // 设置显示区域
    TFT_SET_ADD(0, 0, 159, 159);

    // 使用 DMA 发送数据，短时间内发送整个数组
    TFT_SEND_DATA_DMA(ptr_pic, size);
		TFT_WaitDMA();
 }
	
	

void GC9D01_CS_LOW(void) {
    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET);
}

void GC9D01_CS_HIGH(void) {
    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_SET);
}

void GC9D01_DC_LOW(void) {
    HAL_GPIO_WritePin(SPI1_DC_GPIO_Port, SPI1_DC_Pin, GPIO_PIN_RESET);
}

void GC9D01_DC_HIGH(void) {
    HAL_GPIO_WritePin(SPI1_DC_GPIO_Port, SPI1_DC_Pin, GPIO_PIN_SET);
}

void GC9D01_RESET_LOW(void) {
    HAL_GPIO_WritePin(SPI1_RST_GPIO_Port, SPI1_RST_Pin, GPIO_PIN_RESET);
}

void GC9D01_RESET_HIGH(void) {
    HAL_GPIO_WritePin(SPI1_RST_GPIO_Port, SPI1_RST_Pin, GPIO_PIN_SET);
}

void GC9D01_WriteCommand(uint8_t cmd) {
    GC9D01_DC_LOW();
    GC9D01_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
    GC9D01_CS_HIGH();
}

void GC9D01_WriteData(uint8_t data) {
    GC9D01_DC_HIGH();
    GC9D01_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);
    GC9D01_CS_HIGH();
}

void GC9D01_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    GC9D01_WriteCommand(0x2A); // Column Address Set
    GC9D01_WriteData(x0 >> 8);
    GC9D01_WriteData(x0 & 0xFF);
    GC9D01_WriteData(x1 >> 8);
    GC9D01_WriteData(x1 & 0xFF);

    GC9D01_WriteCommand(0x2B); // Page Address Set
    GC9D01_WriteData(y0 >> 8);
    GC9D01_WriteData(y0 & 0xFF);
    GC9D01_WriteData(y1 >> 8);
    GC9D01_WriteData(y1 & 0xFF);

    GC9D01_WriteCommand(0x2C); // Memory Write
}

void GC9D01_DrawPixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= 160 || y >= 160) return;
    GC9D01_SetAddressWindow(x, y, x, y);
    GC9D01_WriteData(color >> 8);
    GC9D01_WriteData(color & 0xFF);
}

void GC9D01_FillScreen(uint16_t color) {
    GC9D01_SetAddressWindow(0, 0, 160 - 1, 160 - 1);
    for (uint32_t i = 0; i < 160 * 160; i++) {
        GC9D01_WriteData(color >> 8);
        GC9D01_WriteData(color & 0xFF);
    }
}

void GC9D01_Init(void) {

    // Reset the display
    GC9D01_RESET_LOW();
    HAL_Delay(10);
    GC9D01_RESET_HIGH();
    HAL_Delay(120);

    // Initialization sequence
    GC9D01_WriteCommand(0x11); // Sleep out
    HAL_Delay(120);

    GC9D01_WriteCommand(0x3A); // Interface Pixel Format
    GC9D01_WriteData(0x55);    // 16-bit color

    GC9D01_WriteCommand(0x36); // Memory Data Access Control
    GC9D01_WriteData(0x08);    // RGB order

    GC9D01_WriteCommand(0x29); // Display on
}

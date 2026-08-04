#ifndef  _APP_H
#define  _APP_H

//hg25q256.h
#define PAGE_SIZE 256               // 每页大小
#define SECTOR_SIZE 4096            // 每个扇区的大小
#define HG25Q256MW_CMD_READ_ID         0x9F
#define TOTAL_SIZE 51200          // 总共要读取的字节数
#define DISPLAY_CHUNK_SIZE 1  // 缓存显示的数据大小
#define PAGES_PER_READ 100          // 每次读取的页数，这里设置为1页

//sq10.h
enum
{BOTTOM=1,LEFT=2,RIGHT=3,NECK=4,HEAD_NOD=5,HEAD_SHAKE=6};
#define SERVO_ID 	NECK        //你要设置的舵机ID号

#ifdef 	SERVO_ID
#define Serve_Index					(SERVO_ID-1)   //作为数据下标，ID号为1的对应下标为0
#endif

#define SERVO_TEST  0

#define ARRAY_SIZE 9
#define FRAME_HEADER_HOST      0xAB
#define FRAME_HEADER_SLAVE     0xAC
#define SUPER_ID               253
#define SERVO_COUNT            6


//can.h
#define CAN_ID 0x00           // 示例 CAN ID
#define  FRAME_END 0xef

#define CAN_TEST 0
#define CAN_LOOPBACK_TEST 0
//#define CAN_SEND_SERVE_POS	1
#define CAN_DATA_BYTES_PER_FRAME 5  // 有效数据字节
#define CAN_FRAME_BYTES 8           // CAN帧总字节数

//tft.h
//屏幕定义
#define PIC_NUM 18000			//图片数据大小
#define     GREEN        0X07E0	  //绿色
#define     RED          0XF800	  //红色
#define     BLUE         0X001F	  //蓝色
#define     WHITE        0XFFFF	  //白色
#define TFT_COLUMN_NUMBER 160
 #define TFT_LINE_NUMBER 160
#define TFT_COLUMN_OFFSET 0
#define TFT_LINE_OFFSET 0
#define TFT_INIT_SUCCESS  0
#define TFT_INIT_ERROR    -1
#define DISPLAY_STATUS_REGISTER 0x09
#define TOTAL_PIXELS 25600 // 160x160 pixels
#define TOTAL_BYTES 51200   // Total bytes of data
#define PIXEL_MULTIPLIER 2 // 放大倍数

// GPIO 操作宏
#define SPI1_CS_0   HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET)   // CS 引脚置0
#define SPI1_CS_1   HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_SET)     // CS 引脚置1
#define SPI1_RST_0  HAL_GPIO_WritePin(SPI1_RST_GPIO_Port, SPI1_RST_Pin, GPIO_PIN_RESET) // RST 引脚置0
#define SPI1_RST_1  HAL_GPIO_WritePin(SPI1_RST_GPIO_Port, SPI1_RST_Pin, GPIO_PIN_SET)   // RST 引脚置1
#define SPI1_DC_0   HAL_GPIO_WritePin(SPI1_DC_GPIO_Port, SPI1_DC_Pin, GPIO_PIN_RESET)   // DC 引脚置0
#define SPI1_DC_1   HAL_GPIO_WritePin(SPI1_DC_GPIO_Port, SPI1_DC_Pin, GPIO_PIN_SET)     // DC 引脚置1


#endif

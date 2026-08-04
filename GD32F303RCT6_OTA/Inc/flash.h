#ifndef  FLASH_H
#define  FLASH_H
#include "includes.h"


#define OTA_CAN_REQUEST_ID 0x401 // OTA升级请求帧ID
#define OTA_CAN_DATA_ID 0x402 // OTA升级数据帧ID

#define OTA_FLASH_MAX_SIZE      (112*1024)                  // 最大128KB
#define OTA_FLASH_PAGE_SIZE     (2048)                     // GD32F303每页2KB
#define FLASH_SIZE              0x40000U                    // 256KB
#define FLASH_START_ADDR        0x08000000U
#define FLASH_END_ADDR          0x0803FFFFU

#define BOOTLOADER_ADDR         0x08000000U
#define BOOTLOADER_SIZE         0x4000U   //16KB

#define APPLICATION_1_ADDR      0x08004000U
#define APPLICATION_1_SIZE      0x20000U		//128KB

#define APPLICATION_2_ADDR      0x08024000U
#define APPLICATION_2_SIZE      0x1C000U   //112KB

#define FLAG_ADDR               (APPLICATION_2_ADDR + APPLICATION_2_SIZE - 4)  //802 5D10   0x080362C7, flag addr = 0x0803FFFC
#define OTA_FLASH_BASE          APPLICATION_2_ADDR
#define OTA_SECTOR_SIZE         OTA_FLASH_PAGE_SIZE            // 2KB一页

#define STARTUP_NORMAL          0xFFFFFFFFU
#define STARTUP_UPDATE          0xAAAAAAAAU
#define STARTUP_RESET           0x5555AAAAU

extern void flash_rw_test(void);
extern uint32_t read_start_mode(void);

// 写入启动模式标志（4字节，升级区最后4字节，需先擦除所在页）
extern void write_start_mode(uint32_t mode);

// OTA升级片内FLASH擦除（页擦除，4字节对齐，不能跨页）
extern void ota_flash_erase(uint32_t addr, uint32_t len);
// OTA升级写入
// 不能跨页写入，4字节对齐
extern void ota_flash_write(uint32_t addr, const uint8_t* data_buf, uint32_t len);

// OTA升级读取（直接指针访问）
extern void ota_flash_read(uint32_t addr, uint8_t* buf, uint32_t len);


#endif

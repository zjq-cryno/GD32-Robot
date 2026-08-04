#ifndef  FLASH_H
#define  FLASH_H
#include "includes.h"

/*=====用户配置(根据自己的分区进行配置)=====*/
#define BOOTLOADER_SIZE 		0x4000U     // BootLoader 16K
#define Application_Size		0x20000U		///< 应用程序的大小 128K

#define APPLICATION_1_ADDR		0x08004000U		///< 应用程序1的首地址，扇区5首地址
#define APPLICATION_2_ADDR		0x08024000U		///< 应用程序2的首地址，扇区6首地址



#define APPLICATION_1_SIZE		0x20000U		///< 应用程序1的大小
#define APPLICATION_2_SIZE		0x1C000U		///< 应用程序2的大小

#define FLASH_PAGE_SIZE  0x800U         // GD32F303每页2KB
#define OTA_FLAG_ADDR        (APPLICATION_2_ADDR + APPLICATION_2_SIZE - 4) // OTA升级标志地址   0x803FFFC
/*==========================================*/


/* 启动的步骤 */
#define STARTUP_NORMAL 0xFFFFFFFF	///< 正常启动
#define STARTUP_UPDATE 0xAAAAAAAA	///< 升级再启动
#define STARTUP_RESET  0x5555AAAA	///< 恢复出厂,目前没使用


extern void Flash_ErasePage(uint32_t address);
extern void Flash_WriteWord(uint32_t addr, uint32_t word);
extern void Flash_WriteWords(uint32_t addr, uint32_t *buf, uint32_t word_size);
extern void flash_read(uint32_t addr, uint32_t * buf, uint32_t word_size);
extern uint32_t read_start_mode(void);
extern void write_start_mode(uint32_t mode);
extern void move_code(uint32_t dest_addr, uint32_t src_addr,uint32_t size);
extern __asm void MSR_MSP (uint32_t ulAddr);
extern void iap_execute_app (uint32_t app_addr);
extern void Flash_EraseRange(uint32_t start_addr, uint32_t end_addr);
extern void flash_rw_test(void);
#endif

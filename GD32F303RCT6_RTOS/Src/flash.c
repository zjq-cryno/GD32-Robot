#include "includes.h"
#include "app.h"
#include "gd32_adapter.h"
#define TEST_DATA      0x12345678U      // 测试写入数据
void flash_rw_test(void)
{
    uint32_t val;
    int ret;
    uint32_t test_addr = 0x0803F000; // 使用更安全的测试地址
    uint16_t flash_size = *(uint16_t*)0x1FFFF7E0;
		printf("Flash size: %d KB\r\n", flash_size);
    printf("=== Flash test start ===\r\n");
    
    // 1. 系统初始化时设置等待状态
    fmc_set_ws(2); // 必须在系统初始化时设置
    
    // 2. 验证地址
    printf("Test address: 0x%08X\r\n", test_addr);
    printf("Wait states: %d\r\n", FMC_WS & 0x07);
    
    // 3. 解锁FMC
    ret = fmc_unlock();
    if (ret != 0) {
        printf("Unlock failed! ret=%d\r\n", ret);
        printf("FMC_CTL0=0x%08X\r\n", FMC_CTL0);
        printf("FMC_STAT0=0x%08X\r\n", FMC_STAT0);
        return;
    }
    printf("Unlock success! FMC_CTL0=0x%08X\r\n", FMC_CTL0);
    
    // 4. 擦除页
    printf("Erasing page...\r\n");
    ret = fmc_page_erase(test_addr);
    if (ret != 0) {
        printf("Erase failed! ret=%d\r\n", ret);
        printf("FMC_STAT0=0x%08X\r\n", FMC_STAT0);
        fmc_lock();
        return;
    }
    printf("Erase success!\r\n");
    
    // 5. 验证擦除
    val = fmc_read_word(test_addr);
    printf("Post-erase value: 0x%08X\r\n", val);
    
    // 6. 编程数据
    printf("Programming data...\r\n");
    ret = fmc_word_program(test_addr, 0x12345678U);
    if (ret != 0) {
        printf("Program failed! ret=%d\r\n", ret);
        printf("FMC_STAT0=0x%08X\r\n", FMC_STAT0);
        fmc_lock();
        return;
    }
    printf("Program success!\r\n");
    
    // 7. 验证编程
    val = fmc_read_word(test_addr);
    printf("Post-program value: 0x%08X\r\n", val);
    
    if (val == 0x12345678U) {
        printf("FLASH WRITE TEST SUCCESS!\r\n");
    } else {
        printf("FLASH WRITE TEST FAIL!\r\n");
        // 详细诊断
        uint16_t low = *(volatile uint16_t *)test_addr;
        uint16_t high = *(volatile uint16_t *)(test_addr + 2);
        printf("Low 16 bits: 0x%04X, High 16 bits: 0x%04X\r\n", low, high);
        
        // 检查写保护
        uint32_t wp = FMC_WP;
        printf("Write protection: 0x%08X\r\n", wp);
    }
    
    // 8. 上锁
    fmc_lock();
    printf("=== Test end ===\r\n");
}

// 读取启动模式标志（4字节，升级区最后4字节）
uint32_t read_start_mode(void) {
    return fmc_read_word(FLAG_ADDR);
}

// 写入启动模式标志（4字节，升级区最后4字节，需先擦除所在页）
void write_start_mode(uint32_t mode) {
    // 计算标志所在页首地址（2KB对齐）
    uint32_t page_addr = FLAG_ADDR & ~(OTA_FLASH_PAGE_SIZE - 1);

    fmc_unlock();
    fmc_page_erase(page_addr);    // 先擦除整页
    // 只允许4字节对齐写入
    fmc_word_program(FLAG_ADDR, mode);
    fmc_lock();
    printf("write flag success\r\n");
}

// OTA升级片内FLASH擦除（页擦除，4字节对齐，不能跨页）
void ota_flash_erase(uint32_t addr, uint32_t len) {
    // 检查范围
    if ((addr < FLASH_START_ADDR) || (addr + len > FLASH_END_ADDR + 1)) {
        printf("Flash erase out of OTA range! addr=0x%08X len=0x%X\n", addr, len);
        return;
    }
    // 地址对齐检查
    if (addr % OTA_FLASH_PAGE_SIZE != 0) {
        printf("ERROR: Erase address 0x%08X is not 2KB aligned!\n", addr);
        return;
    }

    uint32_t num_pages = (len + OTA_FLASH_PAGE_SIZE - 1) / OTA_FLASH_PAGE_SIZE;
    fmc_unlock();
    for (uint32_t i = 0; i < num_pages; i++) {
        fmc_page_erase(addr + i * OTA_FLASH_PAGE_SIZE);
    }
    fmc_lock();
}

// OTA升级写入
// 不能跨页写入，4字节对齐
void ota_flash_write(uint32_t addr, const uint8_t* data_buf, uint32_t len) {
  // 确保地址4字节对齐
    if (addr % 4 != 0) {
        printf("ERROR: Address 0x%08X not 4-byte aligned!\n", addr);
        return;
    }
    
    // 确保写入长度是4的倍数
    if (len % 4 != 0) {
        printf("ERROR: Write length %u not multiple of 4!\n", len);
        return;
    }
    
    fmc_unlock();
    fmc_set_ws(2); // 设置等待状态（72MHz）
    
    uint32_t words_to_write = len / 4;
    
    for (uint32_t i = 0; i < words_to_write; i++) {
        
        // 组合4字节数据为一个字
        uint32_t word_data = (data_buf[i*4] << 0)   |
                             (data_buf[i*4+1] << 8)  |
                             (data_buf[i*4+2] << 16) |
                             (data_buf[i*4+3] << 24);
        
        // 写入Flash
        int ret = fmc_word_program(addr + i * 4, word_data);
        if (ret != 0) {
            printf("Word program failed at 0x%08X, ret=%d\r\n", addr + i * 4, ret);
            break;
        }
        
        // 验证写入
        uint32_t read_back = fmc_read_word(addr + i * 4);
				printf("read_back 0x%08X\r\n",read_back);
        if (read_back != word_data) {
            printf("Verify failed at 0x%08X: wrote 0x%08X, read 0x%08X\n", 
                   addr + i * 4, word_data, read_back);
            break;
        }
    }
    
    fmc_lock();
}

// OTA升级读取（直接指针访问）
void ota_flash_read(uint32_t addr, uint8_t* buf, uint32_t len) {
    // 读取不要求对齐
    for (uint32_t i = 0; i < len; i++) {
        buf[i] = *(volatile uint8_t *)(addr + i);
    }
}


/*上电/重启 ----->|Bootloader|
                +---------+
                     |
            +--------+--------+
            |                 |
     检查升级标志？          否--> 进入OTA监听
            |是
            v
       搬运升级区到主区
            |
            v
      跳转到主程序区*/

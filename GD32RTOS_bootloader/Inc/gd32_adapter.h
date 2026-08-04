/*
 * 适用于STM32F1 HAL工程环境下，GD32F303RCT6的FMC寄存器和功能适配
 * 允许在STM32F1 HAL项目中用GD32风格的fmc_unlock等函数操作GD32的Flash
 * 仅适配寄存器直接操作，不依赖GD32官方库
 */

#include "stm32f1xx_hal.h"

// GD32F303RCT6的FMC寄存器物理地址（和STM32F1系列FLASH寄存器兼容）
#define FMC_BASE        (0x40022000UL)
#define FMC_STAT        (*(volatile uint32_t *)(FMC_BASE + 0x0C))   // 状态寄存器
#define FMC_ADDR        (*(volatile uint32_t *)(FMC_BASE + 0x14))   // 地址寄存器
#define FMC_KEY0         (*(volatile uint32_t *)(FMC_BASE + 0x04))   // KEYR
#define FMC_OBKEY       (*(volatile uint32_t *)(FMC_BASE + 0x08))   // OBKEYR
#define FMC_WP          (*(volatile uint32_t *)(FMC_BASE + 0x20))   // WRPR
#define FMC_WS  				(*(volatile uint32_t *)(FMC_BASE + 0x00))
#define FMC_STAT0 			(*(volatile uint32_t *)(FMC_BASE + 0x0C))
#define FMC_CTL0 				(*(volatile uint32_t *)(FMC_BASE + 0x10))	
#define FMC_ADDR0  			(*(volatile uint32_t *)(FMC_BASE + 0x14))	
#define FMC_OBCTL  			(*(volatile uint32_t *)(FMC_BASE + 0x1C))	
#define FMC_KEY1  			(*(volatile uint32_t *)(FMC_BASE + 0x44))	
#define FMC_STAT1  			(*(volatile uint32_t *)(FMC_BASE + 0x4C))	
#define FMC_CTL1  			(*(volatile uint32_t *)(FMC_BASE + 0x50))
#define FMC_ADDR1  			(*(volatile uint32_t *)(FMC_BASE + 0x54))
#define FMC_WSEN  			(*(volatile uint32_t *)(FMC_BASE + 0xFC))
#define FMC_PID  			(*(volatile uint32_t *)(FMC_BASE + 0x100))
	
// 控制位定义（GD32与STM32F1兼容）
#define FMC_CTL0_PG      (1U << 0)   // 编程使能
#define FMC_CTL0_PER     (1U << 1)   // 页擦除使能
#define FMC_CTL0_MER     (1U << 2)   // 整片擦除使能
#define FMC_CTL0_OBPG    (1U << 4)   // Option字节编程
#define FMC_CTL0_OBER    (1U << 5)   // Option字节擦除
#define FMC_CTL0_START   (1U << 6)   // 启动
#define FMC_CTL0_LK      (1U << 7)   // 锁定
#define FMC_CTL0_OBWEN   (1U << 9)
#define FMC_CTL0_ERRIE   (1U << 10) 
#define FMC_CTL0_ENDIE   (1U << 12) 

// 状态位定义
#define FMC_STAT_BUSY   (1U << 0)   // 忙
#define FMC_STAT_PGERR  (1U << 2)   // 编程错误
#define FMC_STAT_WPERR  (1U << 4)   // 写保护错误
#define FMC_STAT_ENDF   (1U << 5)   // 操作完成

// 解锁密钥（GD32与STM32F1一致）
// 解锁密钥
#define UNLOCK_KEY0     0x45670123U
#define UNLOCK_KEY1     0xCDEF89ABU

#define FMC_TIMEOUT     0x10000

// 等待状态设置（主频高于48MHz须设置2WS）
static inline void fmc_set_ws(uint32_t ws) {
    FMC_WS = (FMC_WS & ~0x7) | (ws & 0x7);
}

// 等待FMC空闲
static inline int fmc_wait_ready(void) {
    uint32_t timeout = FMC_TIMEOUT;
    while ((FMC_STAT0 & FMC_STAT_BUSY) && --timeout);
    return (FMC_STAT0 & FMC_STAT_BUSY) ? -1 : 0;
}

// 清除所有flag
static inline void fmc_clear_flags(void) {
    FMC_STAT0 |= FMC_STAT_ENDF | FMC_STAT_PGERR | FMC_STAT_WPERR;
}

// 解锁FMC
static inline int fmc_unlock(void) {
    if (FMC_CTL0 & FMC_CTL0_LK) {
        FMC_KEY0 = UNLOCK_KEY0;
        FMC_KEY0 = UNLOCK_KEY1;
			// 验证第一层解锁是否成功
        if (FMC_CTL0 & FMC_CTL0_LK) {
            return -1; // 解锁失败
        }
    }// 第二层解锁 (选项字节操作解锁)
    if (!(FMC_CTL0 & FMC_CTL0_OBWEN)) {
        FMC_OBKEY = UNLOCK_KEY0;
        FMC_OBKEY = UNLOCK_KEY1;
        
        // 验证第二层解锁是否成功
        if (!(FMC_CTL0 & FMC_CTL0_OBWEN)) {
            return -2; // 选项字节解锁失败
        }
    }
    
    return 0; // 解锁成功
}

// 上锁FMC
static inline void fmc_lock(void) {
    FMC_CTL0 |= FMC_CTL0_LK;
}

// 擦除一页（传入页首地址，2KB对齐）
static inline int fmc_page_erase(uint32_t page_addr) {
    if (fmc_wait_ready() < 0) return -1;
    fmc_clear_flags();
    FMC_CTL0 |= FMC_CTL0_PER;
    FMC_ADDR0 = page_addr;
    FMC_CTL0 |= FMC_CTL0_START;
    if (fmc_wait_ready() < 0) return -2;
    FMC_CTL0 &= ~FMC_CTL0_PER;
    // 清除ENDF
    if (FMC_STAT0 & FMC_STAT_ENDF) FMC_STAT0 |= FMC_STAT_ENDF;
    if (FMC_STAT0 & (FMC_STAT_PGERR | FMC_STAT_WPERR)) return -3;
    return 0;
}

// 编程一个32位字（GD32F303仅支持半字编程，需拆成2次16位）
static inline int fmc_word_program(uint32_t addr, uint32_t data) {
    if (fmc_wait_ready() < 0) return -1;
    
    fmc_clear_flags();
    FMC_CTL0 |= FMC_CTL0_PG;
    __DSB(); // 内存屏障
    
    // 写低16位
    *(volatile uint16_t *)addr = (uint16_t)(data & 0xFFFF);
    
    // 关键：添加额外延迟
    for (volatile int i = 0; i < 10; i++);
    
    if (fmc_wait_ready() < 0) {
        FMC_CTL0 &= ~FMC_CTL0_PG;
        return -2;
    }
    
    // 写高16位
    *(volatile uint16_t *)(addr + 2) = (uint16_t)((data >> 16) & 0xFFFF);
    
    // 关键：添加额外延迟
    for (volatile int i = 0; i < 10; i++);
    
    if (fmc_wait_ready() < 0) {
        FMC_CTL0 &= ~FMC_CTL0_PG;
        return -3;
    }
    
    FMC_CTL0 &= ~FMC_CTL0_PG;
    
    // 验证写入结果
    if (*(volatile uint32_t *)addr != data) {
        return -4; // 验证失败
    }
    
    return 0;
}

// 读取一个32位字（地址需4字节对齐）
static inline uint32_t fmc_read_word(uint32_t addr) {
    return *(volatile uint32_t *)addr;
}

// 检查状态标志
static inline uint32_t fmc_flag_get(uint32_t flag) {
    return (FMC_STAT0 & flag);
}

// 清除指定flag
static inline void fmc_flag_clear(uint32_t flag) {
    FMC_STAT0 |= flag;
}

/*
 * 用法示例：
 * fmc_unlock();
 * fmc_set_ws(2); // 72MHz时钟下建议2WS
 * fmc_page_erase(0x08010000U);
 * fmc_word_program(0x08010000U, 0x12345678U);
 * fmc_lock();
 */

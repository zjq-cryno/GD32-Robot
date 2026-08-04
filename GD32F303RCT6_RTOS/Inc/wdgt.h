#ifndef __WDGT_H
#define __WDGT_H

#include "includes.h"

// FWDGT寄存器定义
#define FWDGT_CTL_REG          (0x40003000U)  // 控制寄存器
#define FWDGT_PSC_REG          (0x40003004U)  // 预分频寄存器  
#define FWDGT_RLD_REG          (0x40003008U)  // 重装载寄存器
#define FWDGT_STAT_REG         (0x4000300CU)  // 状态寄存器

// RCU寄存器定义
#define RCU_CTL_REG            (0x40021000U)  // RCU控制寄存器
#define RCU_RSTSCK            (*(volatile uint32_t *)0x40021004U)  // RCU复位和时钟控制寄存器

// FWDGT命令定义
#define FWDGT_CMD_WRITE_ENABLE    0x5555U     // 写使能命令
#define FWDGT_CMD_COUNTER_RELOAD  0xAAAAU     // 计数器重装载命令
#define FWDGT_CMD_ENABLE          0xCCCCU     // 看门狗使能命令

// 预分频系数定义
#define FWDGT_PSC_DIV4        0x00000000U     // 4分频
#define FWDGT_PSC_DIV8        0x00000001U     // 8分频  
#define FWDGT_PSC_DIV16       0x00000002U     // 16分频
#define FWDGT_PSC_DIV32       0x00000003U     // 32分频
#define FWDGT_PSC_DIV64       0x00000004U     // 64分频
#define FWDGT_PSC_DIV128      0x00000005U     // 128分频
#define FWDGT_PSC_DIV256      0x00000006U     // 256分频

// RCU位定义
#define RCU_CTL_IRC40KEN      (1UL << 16)     // IRC40K时钟使能位
#define RCU_CTL_IRC40KSTB     (1UL << 17)     // IRC40K时钟稳定标志位
#define RCU_RSTSCK_WDTRSTF    (1UL << 11)     // 看门狗复位标志

// FWDGT_STAT位定义
#define FWDGT_STAT_RUD        (1UL << 1)      // 重装载值更新状态位
#define FWDGT_STAT_PUD        (1UL << 0)      // 预分频值更新状态位

extern void FWDGT_Init(void);
extern void FWDGT_Feed(void);
extern uint32_t FWDGT_GetStatus(void);
extern uint8_t FWDGT_IsPSCUpdateDone(void);
extern uint8_t FWDGT_IsRLDUpdateDone(void);
extern void Check_Reset_Reason(void);
#endif

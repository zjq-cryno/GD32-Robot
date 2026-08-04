#include "wdgt.h"

/**
  * @brief  FWDGT初始化函数
  * @param  无
  * @retval 无
  */
void FWDGT_Init(void)
{
    volatile uint32_t timeout = 0xFFFFU;
    
    // 1. 使能IRC40K时钟
    *(volatile uint32_t *)RCU_CTL_REG |= RCU_CTL_IRC40KEN;
    
    // 2. 等待IRC40K时钟稳定
    while((*(volatile uint32_t *)RCU_CTL_REG & RCU_CTL_IRC40KSTB) == 0)
    {
        if(timeout > 0) 
            timeout--;
        else
            break;  // 超时退出
    }
    
    // 3. 解锁FWDGT写保护
    *(volatile uint32_t *)FWDGT_CTL_REG = FWDGT_CMD_WRITE_ENABLE;
    
    // 4. 配置预分频器为64分频 (PSC[2:0] = 100)
    *(volatile uint32_t *)FWDGT_PSC_REG = FWDGT_PSC_DIV64;
    
    // 5. 解锁FWDGT写保护（配置重装载值前需要再次解锁）
    *(volatile uint32_t *)FWDGT_CTL_REG = FWDGT_CMD_WRITE_ENABLE;
    
    // 6. 配置重装载值为1000（对应1.6秒超时）
    // 计算公式：Timeout = (1/40kHz) × 预分频系数 × (重装载值 + 1)
    // 对于64分频：Timeout = (1/40000) × 64 × (2000 + 1) ≈ 3.2秒
    *(volatile uint32_t *)FWDGT_RLD_REG = 2000U;
    
    // 7. 重装载计数器（确保计数器从新值开始）
    *(volatile uint32_t *)FWDGT_CTL_REG = FWDGT_CMD_COUNTER_RELOAD;
    
    // 8. 使能看门狗
    *(volatile uint32_t *)FWDGT_CTL_REG = FWDGT_CMD_ENABLE;
}

/**
  * @brief  看门狗喂狗函数
  * @param  无
  * @retval 无
  */
void FWDGT_Feed(void)
{
		printf("喂狗了\r\n");
    // 直接发送重装载命令，无需解锁写保护
    *(volatile uint32_t *)FWDGT_CTL_REG = FWDGT_CMD_COUNTER_RELOAD;
		
}

/**
  * @brief  获取看门狗状态
  * @param  无
  * @retval 状态寄存器值
  */
uint32_t FWDGT_GetStatus(void)
{
    return *(volatile uint32_t *)FWDGT_STAT_REG;
}

/**
  * @brief  检查预分频值是否更新完成
  * @param  无
  * @retval 1:更新完成, 0:更新中
  */
uint8_t FWDGT_IsPSCUpdateDone(void)
{
    return ((*(volatile uint32_t *)FWDGT_STAT_REG & FWDGT_STAT_PUD) == 0);
}

/**
  * @brief  检查重装载值是否更新完成  
  * @param  无
  * @retval 1:更新完成, 0:更新中
  */
uint8_t FWDGT_IsRLDUpdateDone(void)
{
    return ((*(volatile uint32_t *)FWDGT_STAT_REG & FWDGT_STAT_RUD) == 0);
}
void Check_Reset_Reason(void)
{
    // 检查是否看门狗复位
    if (RCU_RSTSCK & RCU_RSTSCK_WDTRSTF)
    {
        // 看门狗复位，可以在这里处理复位后的特殊操作
        // 比如点亮错误指示灯或记录复位次数
        printf("看门狗复位\r\n");
        // 清除复位标志
        RCU_RSTSCK |= RCU_RSTSCK_WDTRSTF;
    }
}


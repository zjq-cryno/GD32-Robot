#ifndef __LOG_H
#define __LOG_H

#include "includes.h"

#define LOG_FLASH_START   0x00100000//0x00204800  // Log存储起始地址,留50个扇区用于存储动画
#define LOG_MAX_TOTAL_LEN (1024 * 1024)  // Log总存储容量：1MB（可根据需求调整）
#define LOG_RECORD_MAX_LEN 128    // 单条Log最大长度（含时间戳、级别）

// 2. Log结构体定义（含级别、长度、内容，确保对齐Flash页大小）
#pragma pack(1)  // 强制1字节对齐
typedef struct {
    uint8_t log_level;         // Log级别：0=INFO，1=WARN，2=ERROR
    uint8_t log_len;           // 实际Log内容长度（字节）
    uint8_t log_content[126];  // Log内容
} LogRecord;
#pragma pack()   // 恢复默认对齐

// CAN消息类型定义（用于上位机区分帧用途）
typedef enum {
    CAN_LOG_START = 0x01,  // 日志传输开始帧
    CAN_LOG_DATA  = 0x02,  // 日志数据帧
    CAN_LOG_END   = 0x03   // 日志传输结束帧
} CanLogMsgType;

// CAN日志数据帧结构（8字节，符合CAN标准帧格式）
typedef struct {
    uint8_t msg_type;      // 消息类型（CanLogMsgType）
    uint8_t log_index;     // 日志序号（第几条日志）
    uint8_t packet_index;  // 分包序号（单条日志可能分多帧）
    uint8_t total_packets; // 总分包数
    uint8_t data[8];       // 有效数据（每帧8字节，可调整）
} CanLogPacket;

// 日志发送触发指令定义
#define LOG_TRIGGER_CAN_ID    0x124       // 触发指令的CAN ID
#define LOG_TRIGGER_DATA      0xAA        // 触发指令的数据内容
#define LOG_TRIGGER_DLC       1           // 触发指令的数据长度

extern volatile uint8_t g_trigger_log_send;

uint8_t Log_StorageInit(void);
void Log_UpdateWriteAddr(void);
uint8_t Log_WriteToFlash_DMA(uint8_t level, const char* p_content);
void ReadAndPrintLogFromFlash(uint32_t start_addr);
uint8_t CAN_SendLogFromFlash(uint32_t log_addr, uint8_t log_index);
uint8_t CAN_SendAllLogsFromFlash(uint32_t start_addr, uint16_t max_logs);
HAL_StatusTypeDef CAN_SendPacket(CAN_HandleTypeDef* hcan, uint32_t std_id, CanLogPacket* pkt);
void CAN_SendLogStatus(uint8_t status);
void ReadSpecificLog(uint32_t addr);
uint32_t Log_FindLastValidLogAddr(void);

#endif

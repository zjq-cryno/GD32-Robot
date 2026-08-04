#include "log.h"


// Log存储状态变量（全局，记录当前写入位置）
uint32_t g_log_current_addr = LOG_FLASH_START;  // 当前Log写入地址
uint8_t g_log_storage_full = 0;     

// 日志发送触发标志（volatile确保中断和主循环可见性）
volatile uint8_t g_trigger_log_send = 0;

/**
 * @brief Log存储初始化（系统启动时调用）
 * @return 0=成功，1=初始化失败
 */
uint8_t Log_StorageInit(void) {
    // 检查初始扇区是否已擦除（擦除后数据为0xFF）
    uint8_t temp_buf[4];
    HG25Q256_Read_Data(temp_buf, LOG_FLASH_START, 4);
    if (temp_buf[0] != 0xFF || temp_buf[1] != 0xFF || temp_buf[2] != 0xFF || temp_buf[3] != 0xFF) {
        // 初始扇区未擦除，执行擦除
        HG25Q256_Erase_Sector(LOG_FLASH_START);
        if (HG25Q256_ReadSR() & 0x01) {
            return 1; // 擦除失败
        }
    }
    
    // 初始化当前写入地址
    g_log_current_addr = LOG_FLASH_START;
    g_log_storage_full = 0;
		//printf("LogRecord 实际大小: %d 字节\n", sizeof(LogRecord));
    return 0;
}


// 检查并更新Log写入地址，处理循环覆盖
void Log_UpdateWriteAddr(void) {
    // 计算下一个Log的起始地址
    uint32_t next_addr = g_log_current_addr + sizeof(LogRecord);
    
    // 检查是否超过Log存储最大范围
    if (next_addr > LOG_FLASH_START + LOG_MAX_TOTAL_LEN) {
        // 存储已满，重置地址到起始位置，覆盖旧数据
        g_log_current_addr = LOG_FLASH_START;
        g_log_storage_full = 1;
        // 擦除当前扇区（首次覆盖时需擦除，避免旧数据干扰）
        HG25Q256_Erase_Sector(g_log_current_addr);
    } else {
        // 检查当前地址是否跨扇区（扇区大小4KB），跨扇区需提前擦除
        if ((g_log_current_addr & (SECTOR_SIZE - 1)) == 0) {
            HG25Q256_Erase_Sector(g_log_current_addr);
        }
        g_log_current_addr = next_addr;
    }
}

/**
 * @brief 写入单条Log到SPI Flash（高速版，DMA写入）
 * @param level: Log级别（0=INFO，1=WARN，2=ERROR）
 * @param p_content: Log内容字符串（需以'\0'结尾）
 * @return 0=成功，1=参数错误，2=DMA初始化失败，3=写入失败
 */
uint8_t Log_WriteToFlash_DMA(uint8_t level, const char* p_content) {
	
    LogRecord log_rec;
    uint16_t content_len;
    uint8_t dma_ret;
    
    // 1. 参数校验
    if (p_content == NULL || level > 2) {
        return 1; // 参数错误
    }
    
    // 2. 填充Log结构体（同基础版）
		memset(&log_rec, 0, sizeof(LogRecord));  // 初始化整个结构体
		log_rec.log_level = level;
		content_len = strlen(p_content);
		log_rec.log_len = (content_len > sizeof(log_rec.log_content) - 1) ? 
                     (sizeof(log_rec.log_content) - 1) : content_len;
    memcpy(log_rec.log_content, p_content, log_rec.log_len);
    log_rec.log_content[log_rec.log_len] = '\0';  // 确保字符串结束
    
    // 3. 检查并擦除当前扇区（保持不变）
    if ((g_log_current_addr & (SECTOR_SIZE - 1)) == 0) {
        HG25Q256_Erase_Sector(g_log_current_addr);
        if (HG25Q256_ReadSR() & 0x01) {
            return 3; // 擦除失败
        }
    }
	
    
    // 4. DMA写入Log到Flash（调用文档中的DMA函数）
    dma_ret = HG25Q256_Send_Data_DMA((uint8_t*)&log_rec, sizeof(LogRecord), g_log_current_addr);
    if (dma_ret != 0) {
        return 2; // DMA写入失败（返回值对应文档中错误码：1=命令失败，2=DMA失败）
    }
    
    // 5. DMA写入后校验（可选）
    LogRecord log_check;
    dma_transfer_complete = 0; // 重置DMA标志（文档中全局变量）
    if (HG25Q256_Read_Data_DMA((uint8_t*)&log_check, sizeof(LogRecord), g_log_current_addr) != 0) {
        return 3; // 校验读取失败
    }

    uint32_t timeout = 10000; // 设定超时时间
		while (!dma_transfer_complete && timeout--) {
				HAL_Delay(1);
		}
		if (timeout == 0) {
				// 读取超时处理
				return 4; 
		}
		if (memcmp(&log_rec, &log_check, sizeof(LogRecord)) != 0) {
        return 5; // 校验不通过
    }
   
   
    
    return 0; // 成功
}

/**
 * @brief 从外部 Flash 读取指定长度数据并打印为字符串
 * @param start_addr：读取起始地址
 * @param length：读取数据长度
 */
void ReadAndPrintLogFromFlash(uint32_t start_addr) {
     LogRecord log_check;
    dma_transfer_complete = 0;
    // 读取Flash数据
    int read_ret = HG25Q256_Read_Data_DMA((uint8_t*)&log_check, sizeof(LogRecord), start_addr);
    if (read_ret != 0) {
        printf("Flash读取失败，错误码:%d\n", read_ret);
        return;
    }
		
    while (!dma_transfer_complete);
		printf("LogRecord 大小=%d\r\n", sizeof(LogRecord));
		
		if (log_check.log_content[log_check.log_len - 1] != '\0') {
				log_check.log_content[log_check.log_len] = '\0';
		}
    // 打印时校验 log_len，避免越界
    if (log_check.log_len <= sizeof(log_check.log_content)) {
        printf("验证读取的Log内容：级别 %d，内容 %s\r\n", 
               log_check.log_level, 
               log_check.log_content);
    } else {
        printf("Log内容长度异常，log_len = %d\r\n", log_check.log_len);
    }
}


/**
 * @brief 从Flash读取单条日志并通过CAN发送
 * @param log_addr：日志在Flash中的起始地址
 * @param log_index：日志序号（用于上位机标识）
 * @return 0=成功，1=读取失败，2=CAN发送失败
 */
uint8_t CAN_SendLogFromFlash(uint32_t log_addr, uint8_t log_index) {
    LogRecord log_rec;
    uint8_t ret;
    
    // 1. 从Flash读取单条日志
    ret = HG25Q256_Read_Data_DMA((uint8_t*)&log_rec, sizeof(LogRecord), log_addr);
    if (ret != 0) {
        printf("读取Flash日志失败，地址：0x%X\r\n", log_addr);
        return 1;
    }
    
    // 2. 计算总数据长度（级别1字节 + 内容长度log_len）
    uint32_t total_data_len = 1 + log_rec.log_len;
    // 计算需要的分包数（每帧传输4字节数据）
    uint8_t total_packets = (total_data_len + 3) / 4;  // 向上取整
    
    // 3. 发送开始帧（通知上位机新日志开始）
    CanLogPacket start_pkt = {
        .msg_type = CAN_LOG_START,
        .log_index = log_index,
        .packet_index = 0,
        .total_packets = total_packets,
        .data = {0}  // 开始帧数据域无效
    };
    if (CAN_SendPacket(&hcan, 0x123, &start_pkt) != HAL_OK) {  // 0x123为CAN发送ID，可自定义
        printf("CAN发送开始帧失败\r\n");
        return 2;
    }
    HAL_Delay(10);  // 短暂延时，避免帧冲突
    
    // 4. 拆分日志数据为多个CAN数据帧并发送
    uint8_t* data_ptr = (uint8_t*)&log_rec.log_level;  // 从log_level开始发送
    for (uint8_t i = 0; i < total_packets; i++) {
        CanLogPacket data_pkt;
        data_pkt.msg_type = CAN_LOG_DATA;
        data_pkt.log_index = log_index;
        data_pkt.packet_index = i;
        data_pkt.total_packets = total_packets;
        
        // 拷贝4字节数据（最后一帧可能不足4字节）
        uint8_t copy_len = (i == total_packets - 1) ? 
                          (total_data_len % 4) : 4;
        if (copy_len == 0) copy_len = 4;  // 整除时取4字节
        memcpy(data_pkt.data, data_ptr + i*4, copy_len);
        
        // 发送数据帧
        if (CAN_SendPacket(&hcan, 0x123, &data_pkt) != HAL_OK) {
            printf("CAN发送数据帧失败，序号：%d\r\n", i);
            return 2;
        }
        HAL_Delay(5);  // 控制发送速率，避免总线拥堵
    }
    
    // 5. 发送结束帧（通知上位机该日志传输完成）
    CanLogPacket end_pkt = {
        .msg_type = CAN_LOG_END,
        .log_index = log_index,
        .packet_index = 0,
        .total_packets = total_packets,
        .data = {0}  // 结束帧数据域无效
    };
    if (CAN_SendPacket(&hcan, 0x123, &end_pkt) != HAL_OK) {
        printf("CAN发送结束帧失败\r\n");
        return 2;
    }
    
    printf("日志%d通过CAN发送完成，共%d帧\r\n", log_index, total_packets + 2);
    return 0;
}

/**
 * @brief 发送所有Flash日志到上位机
 * @param start_addr：日志存储起始地址（LOG_FLASH_START）
 * @param max_logs：最大发送日志数量（避免无限循环）
 * @return 0=全部发送完成，1=读取失败，2=CAN发送失败
 */
uint8_t CAN_SendAllLogsFromFlash(uint32_t start_addr, uint16_t max_logs) {
    uint32_t current_addr = start_addr + sizeof(LogRecord);
    uint8_t log_count = 0;
    
    // 遍历Flash日志区域，直到地址溢出或达到最大数量
    while (current_addr + sizeof(LogRecord) <= LOG_FLASH_START + LOG_MAX_TOTAL_LEN 
           && log_count < max_logs) {
        // 检查当前地址是否为有效日志（通过log_level判断，0-2为有效）
        uint8_t log_level;
        uint8_t ret = HG25Q256_Read_Data_DMA(&log_level, 1, current_addr); 
				if(ret == 0){
					printf("读取成功\r\n");
				}else{
					printf("读取失败\r\n");
				}
					
				printf("current_addr=0x%X\r\n",current_addr);
				printf("log_level=%d\r\n",log_level);
				if (log_level > 2) {
            // 无效日志（未使用的Flash区域），退出循环
            break;
        }
        
        // 发送当前日志
        ret = CAN_SendLogFromFlash(current_addr, log_count);
        if (ret != 0) {
            return ret;
        }
        // 移动到下一条日志
        current_addr += sizeof(LogRecord);
        log_count++;
        HAL_Delay(100);  // 每条日志间隔，给上位机处理时间
    }
    
    printf("所有日志发送完成，共%d条\r\n", log_count);
		CAN_SendLogStatus(0);  // 发送成功状态
    return 0;
}

HAL_StatusTypeDef CAN_SendPacket(CAN_HandleTypeDef* hcan, uint32_t std_id, CanLogPacket* pkt) {
    CAN_TxHeaderTypeDef tx_header;
    uint8_t tx_data[8];
    uint32_t tx_mailbox;
    
    // 配置CAN发送头（标准帧，8字节数据）
    tx_header.StdId = std_id;
    tx_header.ExtId = 0;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.IDE = CAN_ID_STD;
    tx_header.DLC = 8;  // 固定8字节
    tx_header.TransmitGlobalTime = DISABLE;
    
    // 填充发送数据（将CanLogPacket转为8字节数组）
    memcpy(tx_data, pkt, 8);
    
    // 发送CAN帧
    return HAL_CAN_AddTxMessage(hcan, &tx_header, tx_data, &tx_mailbox);
}

void CAN_SendLogStatus(uint8_t status) {
    CanLogPacket status_pkt;
    status_pkt.msg_type = 0x04;  // 自定义状态消息类型
    status_pkt.log_index = 0xFF; // 无效日志序号
    status_pkt.packet_index = 0;
    status_pkt.total_packets = 0;
    status_pkt.data[0] = status; // 0=成功，1=失败
    CAN_SendPacket(&hcan, 0x125, &status_pkt);  // 状态反馈CAN ID=0x125
}
void ReadSpecificLog(uint32_t addr) {
    uint8_t buf[128];
    HG25Q256_Read_Data(buf, 128, addr);
    printf("[读取地址0x%X] 原始数据：", addr);
    for (int i = 0; i < 128; i++) {
        printf("%#2x ", buf[i]);
    }
    printf("\n");
    // 提取log_content（从第6字节开始，即偏移5）
    printf("[解析内容] %s\n", (char*)(buf + 6));
}

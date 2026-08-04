#include "includes.h"  
#include "app.h"

//这是外部SPI Flash的代码

uint8_t A[BLOCK_SIZE]; // 用于读取的缓冲区 A
uint8_t B[BLOCK_SIZE]; // 用于显示的缓冲区 B
uint8_t read_buffer[DISPLAY_CHUNK_SIZE];              // DMA 读取数据的缓冲区
uint8_t display_buffer[DISPLAY_CHUNK_SIZE]; // 存放显示的数据

uint32_t display_blocks = 0;                         // 当前已显示的块数
volatile uint8_t dma_transfer_complete = 0;
volatile uint8_t dma_transfer_complete_spi1 = 0;
// 使能HG25Q256
void HG25Q256_Enable() {
    HAL_GPIO_WritePin(SPI2_CS_GPIO_Port, SPI2_CS_Pin, GPIO_PIN_RESET); // Chip select
}

// 禁用HG25Q256
void HG25Q256_Disable() {
    HAL_GPIO_WritePin(SPI2_CS_GPIO_Port, SPI2_CS_Pin, GPIO_PIN_SET); // Chip disselect
}

// SPI 发送一个字节
void spi2_Transmit_one_byte(uint8_t data) {
    HAL_SPI_Transmit(&hspi2, &data, 1, HAL_MAX_DELAY);
}

// SPI 接收一个字节
uint8_t spi2_Receive_one_byte() {
    uint8_t data;
    HAL_SPI_Receive(&hspi2, &data, 1, HAL_MAX_DELAY);
    return data;
}

// 使能写操作
void HG25Q256_Write_Enable() {
    HG25Q256_Enable();
    spi2_Transmit_one_byte(0x06); // 发送写使能命令
    HG25Q256_Disable();
}

// 读取状态寄存器
uint8_t HG25Q256_ReadSR() {
    uint8_t status;
    HG25Q256_Enable();
    spi2_Transmit_one_byte(0x05); // 发送读取状态寄存器命令
    status = spi2_Receive_one_byte(); // 读取一个字节
    HG25Q256_Disable();
    return status;
}

// 等待设备不忙
void HG25Q256_Wait_Busy() {
    while ((HG25Q256_ReadSR() & 0x01) == 0x01); // 等待BUSY位清空
}

// 擦除一个扇区
void HG25Q256_Erase_Sector(uint32_t address) {

    HG25Q256_Write_Enable(); // 使能写操作
    HG25Q256_Wait_Busy();    // 等待设备空闲

    HG25Q256_Enable();
    spi2_Transmit_one_byte(0x20); // 扇区擦除命令
    spi2_Transmit_one_byte((address >> 16) & 0xFF); // 24位地址
    spi2_Transmit_one_byte((address >> 8) & 0xFF);
    spi2_Transmit_one_byte(address & 0xFF);
    HG25Q256_Disable();

    HG25Q256_Wait_Busy(); // 等待擦除完成
		

}

// 写数据到HG25Q256
void HG25Q256_Write_Page(const uint8_t* buffer, uint32_t address, uint16_t length) {
	
    HG25Q256_Write_Enable(); // 使能写操作
    HG25Q256_Wait_Busy(); // 等待设备空闲

    HG25Q256_Enable();
    spi2_Transmit_one_byte(0x02); // 写入命令
    spi2_Transmit_one_byte((address >> 16) & 0xFF); // 24位地址
    spi2_Transmit_one_byte((address >> 8) & 0xFF);
    spi2_Transmit_one_byte(address & 0xFF);

    for (uint16_t i = 0; i < length; i++) {
        spi2_Transmit_one_byte(buffer[i]); // 逐字节写入数据
    }

    HG25Q256_Disable();
    HG25Q256_Wait_Busy(); // 等待写入完成

}

// 从HG25Q256读取数据
void HG25Q256_Read_Data(uint8_t* buffer, uint32_t address, uint16_t length) {
    HG25Q256_Enable();
    spi2_Transmit_one_byte(0x03); // 读取命令
    spi2_Transmit_one_byte((address >> 16) & 0xFF); // 24位地址
    spi2_Transmit_one_byte((address >> 8) & 0xFF);
    spi2_Transmit_one_byte(address & 0xFF);

    for (uint16_t i = 0; i < length; i++) {
        buffer[i] = spi2_Receive_one_byte(); // 逐字节读取数据
    }

    HG25Q256_Disable();
}

// 读取ID的函数
uint8_t hg25q256mw_read_id(void) {
    uint8_t cmd = HG25Q256MW_CMD_READ_ID; // 读取 ID 的命令
    uint8_t id[3] = {0}; // 用于存储接收到的 ID 数据
    uint8_t address[3] = {0x00, 0x00, 0x00}; // 24 位地址，由3个字节组成
 	
		uint8_t status = HG25Q256_ReadSR();
    HG25Q256_Enable(); // 选择 HG25Q256MW

    // 发送读取 ID 命令
    HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY); 

    // 发送 24 位地址（3 个字节，每个字节为 0x00）
    HAL_SPI_Transmit(&hspi2, address, 3, HAL_MAX_DELAY);

    // 读取 ID 数据
    HAL_SPI_Receive(&hspi2, id, 3, HAL_MAX_DELAY); 

    HG25Q256_Disable(); // 取消选择 HG25Q256MW

    return id[2]; // 返回 ID 的最后一个字节
}

// 通过DMA写数据
uint8_t HG25Q256_Send_Data_DMA(const uint8_t* data, uint32_t size, uint32_t address) {
    HG25Q256_Write_Enable();
		HG25Q256_Wait_Busy();
    HG25Q256_Enable();

    // 构建发送命令
    uint8_t command[4] = {
        0x02,
        (address >> 16) & 0xFF,
        (address >> 8) & 0xFF,
        address & 0xFF
    };

    if (HAL_SPI_Transmit(&hspi2, command, sizeof(command), HAL_MAX_DELAY) != HAL_OK) {
        return 1; // 命令发送失败
    }

    // 发送数据通过DMA
    dma_transfer_complete = 0; // 重置 DMA 完成标志
    if (HAL_SPI_Transmit_DMA(&hspi2, (uint8_t*)data, size) != HAL_OK) {
        return 2; // DMA 发送失败
    }
		//printf("dma_transfer_complete = %d\r\n",dma_transfer_complete);
    // 等待传输完成
    while (!dma_transfer_complete);
		//printf("here\r\n");
    HG25Q256_Disable();
    HG25Q256_Wait_Busy(); // 等待操作完成
    return 0;
}


// 通过DMA读取数据 (使用快速读取)
uint8_t HG25Q256_Read_Data_DMA(uint8_t* buffer, uint32_t size, uint32_t address) {
    HG25Q256_Wait_Busy();
    HG25Q256_Enable();

    // 快速读取命令
    uint8_t command[4] = { 0x0B, (address >> 16) & 0xFF, (address >> 8) & 0xFF, address & 0xFF };

    // 发送快速读取命令和地址
    if (HAL_SPI_Transmit(&hspi2, command, sizeof(command), HAL_MAX_DELAY) != HAL_OK) {
        return 1; // 命令发送失败
    }

    // 发送8个虚拟时钟（dummy clocks）
    uint8_t dummy_clk[1] = { 0x00 }; // 发送一个字节，实际数据内容无关紧要
    if (HAL_SPI_Transmit(&hspi2, dummy_clk, 1, HAL_MAX_DELAY) != HAL_OK) {
        return 2; // 虚拟时钟发送失败
    }

    // 通过DMA接收数据
    dma_transfer_complete = 0; // 重置 DMA 完成标志
    if (HAL_SPI_Receive_DMA(&hspi2, buffer, size) != HAL_OK) {
        return 3; // DMA 接收失败
    }

    // 等待传输完成
    //while (!dma_transfer_complete);
		
    //HG25Q256_Disable();
    return 0; // 成功
}


//用于擦除并写入多页
void HG25Q256_Write_Multiple_Pages(uint32_t start_address, const uint8_t* data, uint32_t total_length) {
    uint32_t bytes_written = 0; // 已写入的字节数
    uint32_t address = start_address; // 当前写入地址

    while (bytes_written < total_length) {
        // 计算当前页要写入的长度
        uint16_t length_to_write = (total_length - bytes_written) >= PAGE_SIZE ? PAGE_SIZE : (total_length - bytes_written);

        // 如果当前地址处于新的扇区，擦除相应的扇区
        if ((address & (SECTOR_SIZE - 1)) == 0) {
            HG25Q256_Erase_Sector(address);
        }
        
        // 写入当前页数据
        HG25Q256_Write_Page(&data[bytes_written], address, length_to_write);

        // 更新已写入字节数和地址
        bytes_written += length_to_write;
        address += bytes_written; // 移动到下一页的开始地址
    }
}

// 写入多张图片到Flash
void WriteMultipleImagesToFlash(void) {
    HG25Q256_Write_Multiple_Pages(IMAGE1_FLASH_ADDR, gImage_1, TOTAL_BYTES);
    HG25Q256_Write_Multiple_Pages(IMAGE2_FLASH_ADDR, gImage_2, TOTAL_BYTES);
}

// 定义数据读取和显示函数
void DisplayImageFromFlash(uint32_t flash_addr) {
		uint8_t line_buffer[320]; // 一行160像素，每个像素2字节 = 320字节
    uint32_t current_addr = flash_addr;
    
    // 设置显示区域为全屏
    
		TFT_SET_ADD(0, 0, TFT_COLUMN_NUMBER - 1, TFT_LINE_NUMBER - 1);
    
    // 逐行读取并显示
    for (int row = 0; row < 160; row++) {
        // 读取一行数据
        dma_transfer_complete = 0;
        HG25Q256_Read_Data_DMA(line_buffer, 320, current_addr);
        
        // 等待DMA完成
        uint32_t timeout = 0xFFFFF;
        while (!dma_transfer_complete && timeout--);
        
        if (timeout == 0) {
            printf("读取超时 at row %d\n", row);
            break;
        }
        
        // 显示当前行
        TFT_SEND_DATA_DMA(line_buffer, 320);
        
        
        current_addr += 320; // 移动到下一行
    }
    
		
}



// 动画显示函数（直接从SPI FLASH读取）
void DisplayAnimationFromFlash(void) {
    //while (1) {
        // 显示第一张图片
        /*DisplayImageFromFlash(IMAGE1_FLASH_ADDR);
			//Picture_Display1(gImage_1, TOTAL_BYTES);
        HAL_Delay(1000); // 显示间隔
        
        // 显示第二张图片  
        DisplayImageFromFlash(IMAGE2_FLASH_ADDR);
			//Picture_Display1(gImage_2, TOTAL_BYTES);
        HAL_Delay(1000);*/
			
				DisplayImageFromFlash(IMAGE1_FLASH_ADDR);
				HAL_Delay(1000);
			
				Picture_Display1(gImage_6, TOTAL_BYTES);
				HAL_Delay(1000);
    //}
}
void Flash_WriteRead_Test(void) {
    uint8_t write_data[FLASH_DATA_LEN];
    uint8_t read_data[FLASH_DATA_LEN];
    uint32_t address = FLASH_ADDR;

    // 填充测试写入数据
    for (uint32_t i = 0; i < FLASH_DATA_LEN; i++) {
        write_data[i] = (uint8_t)(i & 0xFF);
    }

    // 擦除写入扇区，防止写入失败
    HG25Q256_Erase_Sector(address);

    // 写入数据到FLASH
    HG25Q256_Write_Multiple_Pages(address, write_data, FLASH_DATA_LEN);
		

    // 从FLASH读取数据
    HG25Q256_Read_Data(read_data, address, FLASH_DATA_LEN);
	
		
    // 打印写入和读取数据
    printf("写入数据:\n");
    for (uint32_t i = 0; i < FLASH_DATA_LEN; i++) {
        printf("%02X ", write_data[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");

    printf("读取数据:\n");
    for (uint32_t i = 0; i < FLASH_DATA_LEN; i++) {
        printf("%02X ", read_data[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");

    // 简单校验
    if (memcmp(write_data, read_data, FLASH_DATA_LEN) == 0) {
        printf("FLASH写入/读取测试成功！\n");
    } else {
        printf("FLASH写入/读取测试失败！\n");
    }
}
// 简易DMA写入/读取测试
void Flash_DMA_SimpleTest(void)
{
    uint8_t wbuf[256], rbuf[256];
    uint32_t addr = 0x000000;
    uint8_t ret;

    // 填充写入数据
    for (int i = 0; i < 256; i++) wbuf[i] = 0x55 + i;

    printf("擦除扇区...\n");
    HG25Q256_Erase_Sector(addr);

    printf("DMA写入...\n");
    ret = HG25Q256_Send_Data_DMA(wbuf, 256, addr);
    printf("DMA写入返回: %d\n", ret);

    // 写后需等待flash内部写完成
    HG25Q256_Wait_Busy();

    printf("DMA读取...\n");
    memset(rbuf, 0, 256);
    dma_transfer_complete = 0;
    ret = HG25Q256_Read_Data_DMA(rbuf, 256, addr);
    if (ret != 0) {
        printf("DMA读启动失败, ret = %d\n", ret);
        return;
    }
    while (!dma_transfer_complete);

    printf("写入数据:\n");
    for (int i = 0; i < 256; i++) {
        printf("%02X ", wbuf[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n读取数据:\n");
    for (int i = 0; i < 256; i++) {
        printf("%02X ", rbuf[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");

    if (memcmp(wbuf, rbuf, 256) == 0) {
        printf("DMA写入/读取测试成功！\n");
    } else {
        printf("DMA写入/读取测试失败！\n");
    }
}



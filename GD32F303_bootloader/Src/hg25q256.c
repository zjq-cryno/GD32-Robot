#include "includes.h"  
#include "app.h"



uint8_t A[BLOCK_SIZE]; // 用于读取的缓冲区 A
uint8_t B[BLOCK_SIZE]; // 用于显示的缓冲区 B
uint8_t read_buffer[DISPLAY_CHUNK_SIZE];              // DMA 读取数据的缓冲区
//uint8_t display_buffer[DISPLAY_CHUNK_SIZE]; // 存放显示的数据

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
    HG25Q256_Wait_Busy();
    HG25Q256_Write_Enable();
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

    // 等待传输完成
    while (!dma_transfer_complete);

    HG25Q256_Disable();
    HG25Q256_Wait_Busy(); // 等待操作完成
    return 0;
}

// 通过DMA读取数据
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
        address += PAGE_SIZE; // 移动到下一页的开始地址
    }
}
// 定义数据读取和显示函数
void ReadAndDisplayDataFromHG25Q256(uint32_t start_address) {
    uint32_t bytes_read = 0;
    uint32_t total_read_bytes = 0;
    uint32_t remaining_size = TOTAL_SIZE;

    while (remaining_size > 0) {
        uint32_t current_read_size = (remaining_size < (PAGES_PER_READ * PAGE_SIZE)) ? remaining_size : (PAGES_PER_READ * PAGE_SIZE);

        // 通过 DMA 读取数据
        if (HG25Q256_Read_Data_DMA(read_buffer, current_read_size, start_address + bytes_read) != 0) {
            // 错误处理，读取失败
            return; // 退出函数
        }

        // 数据读取完成后，拷贝到缓存
        //memcpy(&display_buffer[total_read_bytes], read_buffer, current_read_size);
        total_read_bytes += current_read_size;

        // 更新已读取字节与剩余字节
        bytes_read += current_read_size;
        remaining_size -= current_read_size;
        // 每当读取到 25600 字节时，将数据展示到屏幕上
        if (total_read_bytes >= DISPLAY_CHUNK_SIZE) {
            Picture_Display1(read_buffer, total_read_bytes);
            total_read_bytes = 0; // 清空以进行下次显示
            display_blocks++; // 增加显示块数
        }
    }

    // 显示剩余未显示的数据
    if (total_read_bytes > 0) {
        //Picture_Display1(display_buffer, total_read_bytes);
        display_blocks++;
    }
}


//B数组发送 A数组读取 

// 读取和显示图像
void ReadAndDisplayImage(uint32_t start_address,uint8_t n) {
    uint32_t current_address = start_address+BLOCK_SIZE; // 直接在这里声明并初始化
    display_blocks = 0;
	
	Picture_Display1(B, BLOCK_SIZE); // 显示 B 缓冲区
	HG25Q256_Read_Data_DMA(A, BLOCK_SIZE, current_address);
	current_address += BLOCK_SIZE;
	display_blocks++;
	
	while((dma_transfer_complete==0)||(dma_transfer_complete_spi1==0));
	Picture_Display1(A, BLOCK_SIZE);
	HG25Q256_Read_Data_DMA(B, BLOCK_SIZE, current_address); 
	current_address += BLOCK_SIZE;
	display_blocks++;

	while((dma_transfer_complete==0)||(dma_transfer_complete_spi1==0));
	Picture_Display1(B, BLOCK_SIZE); 
	HG25Q256_Read_Data_DMA(A, BLOCK_SIZE, current_address);
	current_address += BLOCK_SIZE;
	display_blocks++;
	
	while((dma_transfer_complete==0)||(dma_transfer_complete_spi1==0));
	//while((dma_transfer_complete!=0)&&(dma_transfer_complete_spi1!=0));
	Picture_Display1(A, BLOCK_SIZE); 
	HG25Q256_Read_Data_DMA(B, BLOCK_SIZE, 0x0000d000*n); 
	while((dma_transfer_complete==0)||(dma_transfer_complete_spi1==0));
	
}




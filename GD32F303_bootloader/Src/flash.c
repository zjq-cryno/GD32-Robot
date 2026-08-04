#include "includes.h"
#include "gd32_adapter.h"

//这是内部Flash的代码，ota升级操作的是内部Flash

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

// 擦除fmc地址从0x08024000到0x0803FFFF范围的所有2KB页
void Flash_EraseRange(uint32_t start_addr, uint32_t end_addr)
{
    const uint32_t PAGE_SIZE = 0x800; // 2KB
    // 对齐起始地址到2KB页首
    uint32_t addr = start_addr & ~(PAGE_SIZE - 1);

    fmc_unlock();
    fmc_set_ws(2); // 主频72MHz建议2WS

    while (addr <= end_addr) {
        int ret = fmc_page_erase(addr);
        if (ret != 0) {
            printf("Erase failed at 0x%08X, ret=%d\r\n", addr, ret);
            // 可以选择break或继续
        }
        addr += PAGE_SIZE;
    }
		printf("Erase success\r\n");
    fmc_lock();
}
// 擦除页
void Flash_ErasePage(uint32_t address)
{
    fmc_unlock();
    fmc_set_ws(2); // 推荐72MHz用2WS
    fmc_page_erase(address);
    fmc_lock();
}

/* 写一个word到指定地址 */
void Flash_WriteWord(uint32_t addr, uint32_t word)
{
		fmc_unlock();
    fmc_set_ws(2); // 推荐72MHz用2WS
    fmc_word_program(addr, word);
    fmc_lock();
}
/* 批量写入word数组 */
void Flash_WriteWords(uint32_t addr, uint32_t *buf, uint32_t word_size)
{
    fmc_unlock();
    fmc_set_ws(2);
		uint32_t i;
		
    for (i = 0; i < word_size; i++) {
        fmc_word_program(addr + (i * 4), buf[i]);
				//if(i == 0)
					//printf("writebuf[0]=0x%08X\r\n",buf[0]);
    }
		//uint32_t j=word_size-1;
		//printf("writeaddr :0x%08X, buf[%d] :0x%08X\r\n",addr,j,buf[j]);
    fmc_lock();
}

void flash_read(uint32_t addr, uint32_t * buf, uint32_t word_size)
{
		for (uint32_t i = 0; i < word_size; i++) {
        buf[i] = fmc_read_word(addr + (i * 4));
				//printf("readbuf[%d]=0x%08X\r\n",i,buf[i]);
    }
		
}

// 读取启动模式 
uint32_t read_start_mode(void)
{
	uint32_t mode = 0;
	
	flash_read((APPLICATION_2_ADDR + APPLICATION_2_SIZE - 4), &mode, 1);
	
	return mode;
}


/* 写入启动模式（建议先擦除最后1页） */
void write_start_mode(uint32_t mode)
{
    uint32_t addr = APPLICATION_2_ADDR + APPLICATION_2_SIZE - 4;
    Flash_ErasePage(addr & ~(0x800 - 1)); // 2KB页对齐擦除
    Flash_WriteWord(addr, mode);
}

/*
 * @bieaf 进行程序的覆盖
 * @detail 1.擦除目的地址
 *         2.源地址的代码拷贝到目的地址
 *         3.擦除源地址
 *
 * @param  搬运的源地址
 * @param  搬运的目的地址
 * @return 搬运的程序大小
 */
void move_code(uint32_t dest_addr, uint32_t src_addr,uint32_t size)
{
	  
	uint32_t temp[256];  // 256 words, 1024 bytes buffer
	uint32_t i;
		/*1.擦除目的地址*/
	printf("> start erase application 1 sector......\r\n");
	uint32_t page_addr = APPLICATION_1_ADDR;   // 从0x08004000开始
	uint32_t page_num = (APPLICATION_1_SIZE + FLASH_PAGE_SIZE - 1)/FLASH_PAGE_SIZE;//计算擦除的页数
	for (i = 0; i < page_num; i++) {
				Flash_ErasePage(page_addr);
				printf("Erased page at 0x%08X\r\n", page_addr);  // 添加调试输出
				page_addr += FLASH_PAGE_SIZE;  
  }

	printf("> erase application 1 success......\r\n");
	
	/*2.开始拷贝*/	

	printf("> start copy......\r\n");

	uint32_t bytes_left = size;
	uint32_t src = src_addr;
	uint32_t dst = dest_addr;
	
	while (bytes_left > 0) {
			// 本次拷贝的字节数（最大1024字节，但不超过剩余字节数）
			uint32_t this_copy = bytes_left > sizeof(temp) ? sizeof(temp) : bytes_left;
			
			// 确保按实际需要处理
			uint32_t actual_bytes = this_copy;
			
			// 计算需要读取的字数（向上取整）
			uint32_t read_word_count = (this_copy + 3) / 4;
			
			// 计算需要写入的字数（向下取整）
			uint32_t write_word_count = this_copy / 4;
			
			// 先全填0xFF
      memset(temp, 0xFF, write_word_count * 4);
			
			// 读取源数据
			flash_read(src, temp, read_word_count);
			
			// 处理最后不足4字节的情况
			if (this_copy % 4 != 0) {
					// 只写入完整字部分
					if (write_word_count > 0) {
							Flash_WriteWords(dst, temp, write_word_count);
					}
					
					// 处理最后不足4字节的部分
					uint32_t partial_bytes = this_copy % 4;
					uint32_t partial_addr = dst + (write_word_count * 4);
					
					// 创建一个临时字缓冲区
					uint32_t last_word = 0xFFFFFFFF;
					uint8_t *last_word_ptr = (uint8_t *)&last_word;
					
					// 复制有效字节
					uint8_t *src_ptr = (uint8_t *)(temp + write_word_count);
					for (uint32_t b = 0; b < partial_bytes; b++) {
							last_word_ptr[b] = src_ptr[b];
					}
					
					// 写入最后一个字
					Flash_WriteWord(partial_addr, last_word);
			} else {
					// 完整写入所有字
					Flash_WriteWords(dst, temp, write_word_count);
			}
			
			// 更新位置和剩余字节
			src += this_copy;
			dst += this_copy;
			bytes_left -= this_copy;
			
			printf("Copied %u bytes (%u%%)\r\n", this_copy, 100 * (size - bytes_left) / size);
	}
	
	printf("> copy finish......\r\n");
	
	/*3.擦除源地址*/
	
	printf("> start erase application 2 sector......\r\n");
	
	//擦除
	uint32_t erase_addr = APPLICATION_2_ADDR;
	page_num = (APPLICATION_2_SIZE + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE;
	for (i = 0; i < page_num; i++) {
				
			uint32_t current_page = erase_addr + (i * FLASH_PAGE_SIZE);
			Flash_ErasePage(current_page);
			printf("Erased source page at 0x%08X\r\n", current_page);
  }
	
	printf("> erase application 2 success......\r\n");
		
}

// 采用汇编设置栈的值 
__asm void MSR_MSP (uint32_t ulAddr) 
{
    MSR MSP, r0 			                   //set Main Stack value
    BX r14
}




typedef void (*jump_func)(void);
void iap_execute_app (uint32_t app_addr)
{
	jump_func jump_to_app; 
	
	printf("* (__IO uint32_t *)app_addr = 0x%08X\r\n", *(__IO uint32_t *)app_addr);
  
	
	if ( ( ( * ( __IO uint32_t * ) app_addr ) & 0x2FFE0000 ) == 0x20000000 )	//检查栈顶地址是否合法.
	{ 
		printf("stack is legal\r\n");
		
		jump_to_app = (jump_func) * ( __IO uint32_t *)(app_addr + 4);			//用户代码区第二个字为程序开始地址(复位地址)		
		
		MSR_MSP( * ( __IO uint32_t * ) app_addr );								//初始化APP堆栈指针(用户代码区的第一个字用于存放栈顶地址)
		
		jump_to_app();															//跳转到APP.
	}
	
	printf("stack is illegal\r\n");
}

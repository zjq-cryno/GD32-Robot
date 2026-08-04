#include "includes.h"
#include "app.h"

uint8_t angle_time_single[ARRAY_SIZE]={0xAB, 0x00, 0x09, 0x41, 0x00, 0x00, 0x00, 0x01};//修改舵机ID为0的为目标角度和转动时间的写指令帧；
uint8_t angle_speed_single[ARRAY_SIZE]={0xAB, 0x00, 0x09, 0x42, 0x00, 0x00, 0x01, 0x00};//修改舵机ID为0的为目标角度和转动速度的写指令帧；
uint8_t Current_threshold_array[ARRAY_SIZE]={0xAB,0x03,0x09,0x32,0x02,0x00,0x01,0x00};//修改舵机ID为3的电流保护阈值为1A，检测时间为1s的写指令帧；
uint8_t setID_array[7]={0xAB, 0xFD, 0x07, 0x10, 0x00, 0x00};//修改舵机ID为的写指令帧
uint8_t servoIDs[SERVO_COUNT] = {1, 2, 3, 4, 5, 6};
uint8_t save_tx_frame[9] = {0xAB, 0xFD, 0x09, 0x2E, 0xF1, 0xF2, 0xF3, 0xF4};

int16_t g_servo_current_angle[SERVO_COUNT] = {0}; // 角度缓存：存储每个舵机解析后的当前角度（0.1°）

uint8_t g_checksum;
int16_t position;
uint8_t posframe_checksum;

uint16_t cur_current;

// 添加全局变量
uint8_t emergency_flags[SERVO_COUNT] = {0}; // 舵机紧急状态标志
uint8_t servo_status[SERVO_COUNT] = {0};    // 舵机当前状态
volatile uint8_t current_servo_index = 0;// 当前检测的舵机索引

// 全局变量
volatile uint8_t servo_status_check_flag = 0;
	
uint8_t angle_speed[6][9]={{0xAB, 0x00, 0x09, 0x42, 0x00, 0x00, 0x01, 0x00},{0xAB, 0x00, 0x09, 0x42, 0x00, 0x00, 0x01, 0x00},{0xAB, 0x00, 0x09, 0x42, 0x00, 0x00, 0x01, 0x00},
{0xAB, 0x00, 0x09, 0x42, 0x00, 0x00, 0x01, 0x00},{0xAB, 0x00, 0x09, 0x42, 0x00, 0x00, 0x01, 0x00},{0xAB, 0x00, 0x09, 0x42, 0x00, 0x00, 0x01, 0x00}};


Servo more_servos[6]={{.id=BOTTOM},{.id=LEFT},{.id=RIGHT},{.id=NECK},{.id=HEAD_NOD},{.id=HEAD_SHAKE}};
Servo my_servo = {
        .id = 1,           // 舵机ID
        
};
bool update_servo[6] = {false,false,false,false,false,false};

/**
 * @brief 更新数组的校验和
 * @param array 需要更新校验和的数组
 * @retval 无
 */
void updateChecksum(uint8_t array[ARRAY_SIZE]) {
    uint8_t cks1 = 0xAB; // 主机帧头
    uint8_t cks2 = 0xAC; // 从机帧头
    
    for (int i = 0; i < (ARRAY_SIZE - 1); i++) {
        cks1 += array[i];
        cks2 += cks1;
    }

    // 更新最后一位校验值
    array[ARRAY_SIZE - 1] = cks1 + cks2;
}
/**
 * @brief 根据十进制数修改数组的第5位和第6位
 * @param array 需要修改的数组
 * @param decimalValue 要设置的十进制值（支持负数）
 * @retval 无
 */
void modifyArrayWithDecimal(uint8_t array[ARRAY_SIZE], int32_t decimalValue) {
    // 如果输入是负数，转换为无符号数
    if (decimalValue < 0) {
        decimalValue = 65536 + decimalValue; // 计算补码
    }

    // 转换为低位在前的16进制
    array[4] = (uint8_t)(decimalValue & 0xFF);       // 第五位
		
    array[5] = (uint8_t)(decimalValue >> 8) & 0xFF;  // 第六位

    // 更新校验位
    updateChecksum(array);
}
/**
 * @brief S20 Pro舵机测试函数
 * @retval 无
 */
void S20_Pro()
{
		
		int j;
    //#define CHECK_BREAK(step) do{for(j=0;j<(step)/10;j++){HAL_Delay(10);if(check_shutdown())return;}}while(0)
		#define CHECK_BREAK(step) do{for(j=0;j<(step)/10;j++){HAL_Delay(10);if(check_current())return;}}while(0)
		servo_angle_speed(&huart5, BOTTOM, 0, 22, angle_speed[BOTTOM-1]);
		//servo_angle_speed(&huart5, LEFT, 0, 60, angle_speed[LEFT-1]);
		//servo_angle_speed_Motor_mode(&huart5, LEFT, 0, 30);
	
		//servo_angle_speed(&huart5, RIGHT, 0, 60, angle_speed[RIGHT-1]);
		//servo_angle_speed(&huart5, NECK, 0, 22, angle_speed[NECK-1]);
		//servo_angle_speed(&huart5, HEAD_NOD, 0, 11, angle_speed[HEAD_NOD-1]);
		//servo_angle_speed(&huart5, HEAD_SHAKE, 0, 6, angle_speed[HEAD_SHAKE-1]);
		CHECK_BREAK(2000);
		servo_angle_speed(&huart5, BOTTOM, -45, 22, angle_speed[BOTTOM-1]);
		//servo_angle_speed(&huart5, LEFT, -120, 60, angle_speed[LEFT-1]);
		//servo_angle_speed_Motor_mode(&huart5, LEFT, -120, 30);
	
		//servo_angle_speed(&huart5, RIGHT, -120, 60, angle_speed[RIGHT-1]);
		//servo_angle_speed(&huart5, NECK,-45, 22, angle_speed[NECK-1]);
		//servo_angle_speed(&huart5, HEAD_NOD, -22, 11, angle_speed[HEAD_NOD-1]);
		//servo_angle_speed(&huart5, HEAD_SHAKE, -12, 6, angle_speed[HEAD_SHAKE-1]);
		CHECK_BREAK(2000);
		servo_angle_speed(&huart5, BOTTOM, 0, 22, angle_speed[BOTTOM-1]);
		//servo_angle_speed(&huart5, LEFT, 0, 60, angle_speed[LEFT-1]);
		//servo_angle_speed_Motor_mode(&huart5, LEFT, 0, 30);
		//servo_angle_speed(&huart5, RIGHT, 0, 60, angle_speed[RIGHT-1]);
		//servo_angle_speed(&huart5, NECK, 0, 22, angle_speed[NECK-1]);
		//servo_angle_speed(&huart5, HEAD_NOD, 0, 11, angle_speed[HEAD_NOD-1]);
		//servo_angle_speed(&huart5, HEAD_SHAKE, 0, 6, angle_speed[HEAD_SHAKE-1]);
		CHECK_BREAK(2000);
		servo_angle_speed(&huart5, BOTTOM, 45, 22, angle_speed[BOTTOM-1]);
		//servo_angle_speed(&huart5, LEFT, 120, 60, angle_speed[LEFT-1]);
		//servo_angle_speed_Motor_mode(&huart5, LEFT, 120, 30);
		//servo_angle_speed(&huart5, RIGHT, 120, 60, angle_speed[RIGHT-1]);
		//servo_angle_speed(&huart5, NECK, 45, 22, angle_speed[NECK-1]);
		//servo_angle_speed(&huart5, HEAD_NOD, 22, 11, angle_speed[HEAD_NOD-1]);
		//servo_angle_speed(&huart5, HEAD_SHAKE, 12, 6, angle_speed[HEAD_SHAKE-1]);
		CHECK_BREAK(2000);

	

	
}
// 解析舵机应答帧
void parse_servo_response(uint8_t *frame, uint8_t length) 
{
    uint8_t id = frame[1];   // 舵机ID
    uint8_t cmd = frame[3];  // 命令字
		uint8_t idx = id - 1;    // 数组索引

		if (id < 1 || id > SERVO_COUNT) return; // ID范围检查

    switch (cmd) {
				case 0x02: // 查询舵机状态
						printf("Query the status of the servo OK\r\n");
            break; 
				case 0x03: // 查询出厂ID
						printf("factory ID is 0x%02X\r\n",frame[1]);
            break;  
        case 0x10: // ID修改应答
            printf("ID changed to: 0x%02X\r\n", id);
            break;
				case 0x11: // 设置波特率
						printf("set baud OK\r\n");
            break; 
				case 0x13: // 角度调整
						printf("angle_modify OK\r\n");
            break;		
				case 0x14: // 设置角度限制
						printf("angle_limits set OK\r\n");
            break;       
				case 0x15: // 设置应答延时时间
						printf("response delayus set OK\r\n");
            break;  
				case 0x16: // 设置最大转动速度
						printf("max rotate speed set OK\r\n");
            break; 
				case 0x18: // 设置最大PWM输出
						printf("max PWM output set OK\r\n");
            break; 
				case 0x19: // 设置控制精度
						printf("control accuracy set OK\r\n");
            break; 
        case 0x2E: // 参数保存应答
            printf("Parameters saved\r\n");
						break;
				case 0x2F: //恢复出厂设置
						printf("Restore factory settings\r\n");
            break;
			
				case 0x30: //堵转保护检测时间
						printf("locked-rotor protection OK\r\n");
						break;

				case 0x33: //修改温度保护阈值
						printf("set Temperature protection threshold OK\r\n");
						break;
				case 0x40: // 修改扭矩状态
						printf("Torque status set OK\r\n");
            break;
				case 0x41: // 设置角度和转动时间
            printf("set angle and rotate time OK\r\n");
            break;				
				case 0x42: // 设置角度和转动速度
						//printf("%d servo set angle and rotate speed OK\r\n",id);
						for(int i = 0; i < frame[2]; i++) {
								printf("%02X ", frame[i]);
						}
            break;
				case 0x44: // 设置角度和转动速度(电机模式使用)
						//printf("%d servo set angle and rotate speed OK（Motor mode）\r\n",id);
						for(int i = 0; i < frame[2]; i++) {
								printf("%02X ", frame[i]);
						}
            break;
				case 0x47: // 舵机模式设置
						printf("set mode OK\r\n");
            break;         
       
				case 0x50: // 当前状态查询应答
            if (length == 6) {
                servo_status[idx] = frame[4]; // 保存状态字节
                
                // 检测堵转(bit0)或温度保护(bit3)
                if ((frame[4] & 0x01) || (frame[4] & 0x08)) {
                    emergency_flags[idx] = 1; // 设置紧急标志
                }
								
            }
            break;
				case 0x51: // 当前位置查询应答
            if (length == 7) {
                position = (frame[5] << 8) | frame[4]; // 小端格式
                printf("servo %d current position: %d\r\n", id, position / 10); // 单位0.1°
            }
            break;	
				case 0x53: // 查询舵机当前电流
						cur_current = (frame[5] << 8) | frame[4];  //单位为100mA
						//printf("id = %d, frame[4] = 0x%02X, frame[5] = 0x%02X,cur_current = %d A\r\n",id,frame[4],frame[5],cur_current/10);
				
						for(int i = 0; i < frame[2]; i++) {
								printf("%02X ", frame[i]);
						}
						break;
				case 0x58: // 马达温度查询
            if (length == 7) {
                int16_t temp = (frame[5] << 8) | frame[4]; // 小端格式
                
                // 检测温度超过阈值
                if (temp > OVER_TEMP_THRESHOLD) {
                    emergency_flags[idx] = 1; // 设置紧急标志
                }
            }
            break;
        case 0x80: // 异常应答
            printf("Error! Code: 0x%02X\r\n", frame[4]);
            break;				
        default:
            printf("Unknown command: 0x%02X\r\n", cmd);
    }
}
/**
 * @brief 设置舵机上电动作和角度校准（手动设置原点中位）
 * @param huart UART句柄指针（如&huart5）
 * @param servo_id 舵机ID（1-250，需与实际舵机ID匹配）
 * @param power_on_angle 上电动作目标角度(度)：建议填0（对应校准后的中位）
 * @retval 无
 * @note 1. 执行前需将舵机手动转到机械原点位置；
 *       2. 函数会将当前机械位置设为电气中位，并配置上电后自动回归该位置；
 *       3. 所有参数会保存到舵机Flash，掉电不丢失。
 */
void configure_servo_power_on_action(UART_HandleTypeDef *huart, uint8_t servo_id, int16_t power_on_angle) {
    uint8_t tx_frame[12] = {0}; // 临时存储命令帧，最大长度13（含校验），预留足够空间
    uint8_t tx_len = 0;         // 实际命令帧长度

    /* -------------------------- 步骤1：手动校准原点中位（将当前位置设为中位） -------------------------- */
    // 命令帧格式（协议3.2.4.2例子3）：AB + ID + 0x07 + 0x13 + A5 A5 + 校验和
    tx_frame[0] = 0xAB;                  // 帧头
    tx_frame[1] = servo_id;              // 舵机ID
    tx_frame[2] = 0x07;                  // 长度（帧头+ID+长度+命令+数据+校验 = 1+1+1+1+2+1=7）
    tx_frame[3] = 0x13;                  // 命令字：角度调整
    tx_frame[4] = 0xA5;                  // 数据位1：0xA5（小端存储，16位数据0xA5A5的低位）
    tx_frame[5] = 0xA5;                  // 数据位2：0xA5（16位数据0xA5A5的高位）
    tx_len = 7;                          // 命令帧长度（含校验位，此时未计算校验，先填总长度）
    
    // 计算校验和
		tx_frame[6] = calculate_checksum(tx_frame, tx_len);
    // 发送命令：设置当前位置为中位
    HAL_UART_Transmit(huart, tx_frame, tx_len, HAL_MAX_DELAY);
    HAL_Delay(50); // 延迟50ms，确保舵机完成中位校准（协议建议延迟≥10ms，实际可根据舵机响应调整）

    /* -------------------------- 步骤2：配置上电动作（上电后转到目标角度，默认0°即中位） -------------------------- */
    // 系统配置数据解析（协议3.2.3.2）：
    // - Bit[1-0] = 0x03：上电动作=转到0°（校准后的中位）
    // - Bit[3] = 0x01：开启写指令应答（便于确认配置成功）
    // - 其他Bit默认0，故32位配置数据为 0x00000008（Bit3=1） + 0x00000003（Bit1-0=3） = 0x0000000B
    uint32_t sys_config = 0x0000000B; 
    
    // 命令帧格式：AB + ID + 0x09 + 0x12 + 配置数据（4字节，小端） + 校验和
    tx_frame[0] = 0xAB;                  // 帧头
    tx_frame[1] = servo_id;              // 舵机ID
    tx_frame[2] = 0x09;                  // 长度（1+1+1+1+4+1=9）
    tx_frame[3] = 0x12;                  // 命令字：系统配置
    // 配置数据（小端存储：低字节在前）
    tx_frame[4] = (uint8_t)(sys_config & 0xFF);        // 字节0：0x0B
    tx_frame[5] = (uint8_t)((sys_config >> 8) & 0xFF); // 字节1：0x00
    tx_frame[6] = (uint8_t)((sys_config >> 16) & 0xFF); // 字节2：0x00
    tx_frame[7] = (uint8_t)((sys_config >> 24) & 0xFF); // 字节3：0x00
    tx_len = 9;                          // 命令帧长度
    
    // 计算校验和（填充到tx_frame[8]）
    tx_frame[8] = calculate_checksum(tx_frame, tx_len);
    // 发送命令：配置上电动作
    HAL_UART_Transmit(huart, tx_frame, tx_len, HAL_MAX_DELAY);
    HAL_Delay(50); // 延迟50ms，确保舵机完成系统配置

    /* -------------------------- 步骤3：保存参数（防止掉电丢失中位和上电动作配置） -------------------------- */
    // 命令帧格式（协议3.2.11例子1）：AB + ID + 0x09 + 0x2E + F1 F2 F3 F4 + 校验和
    tx_frame[0] = 0xAB;                  // 帧头
    tx_frame[1] = servo_id;              // 舵机ID
    tx_frame[2] = 0x09;                  // 长度（1+1+1+1+4+1=9）
    tx_frame[3] = 0x2E;                  // 命令字：保存用户数据
    tx_frame[4] = 0xF1;                  // 数据位1：固定填充F1
    tx_frame[5] = 0xF2;                  // 数据位2：固定填充F2
    tx_frame[6] = 0xF3;                  // 数据位3：固定填充F3
    tx_frame[7] = 0xF4;                  // 数据位4：固定填充F4
    tx_len = 9;                          // 命令帧长度
    
    // 计算校验和（填充到tx_frame[8]）
    tx_frame[8] = calculate_checksum(tx_frame, tx_len);
    // 发送命令：保存参数到Flash
    HAL_UART_Transmit(huart, tx_frame, tx_len, HAL_MAX_DELAY);
    HAL_Delay(100); // 保存Flash需更长延迟（协议建议≥100ms）

    /* -------------------------- 步骤4：（可选）验证配置结果（通过解析应答帧确认） -------------------------- */
    // 注：若需验证，需在UART接收中断中解析舵机应答（参考parse_servo_response函数）
    // 例如：应答帧为AC + servo_id + 0x05 + 0x13 + 校验（中位设置成功）
    //       应答帧为AC + servo_id + 0x05 + 0x12 + 校验（上电动作配置成功）
    //       应答帧为AC + servo_id + 0x05 + 0x2E + 校验（参数保存成功）
    printf("Servo %d origin calibration and power-on action configured successfully!\r\n", servo_id);
}
/**
 * @brief 设置舵机温度保护阈值
 * @param huart UART句柄指针
 * @param servo_id 舵机ID
 * @param 温度保护阈值
 * @retval 无
 */
void temperature_protection_threshold(UART_HandleTypeDef *huart, uint8_t servo_id, int16_t threshold_val)
{
		uint8_t tx_frame[7] = {0xAB, servo_id, 0x07, 0x33, (int8_t)(threshold_val & 0xFF), (int8_t)((threshold_val >> 8) & 0xFF)};

    // 计算校验和并添加到命令末尾
    tx_frame[6] = calculate_checksum(tx_frame, 7);
    uint8_t tx_frame_len=sizeof(tx_frame)/sizeof(tx_frame[0]);
    // 发送命令
    HAL_UART_Transmit(huart, tx_frame, tx_frame_len, HAL_MAX_DELAY);
    HAL_Delay(10);
		// 第二步: 保持参数，防止掉电丢失
		save_servo_config(huart, servo_id);
}

/**
 * @brief 单个舵机零点校准：读取当前角度并设为零点（掉电保存）
 * @param huart UART句柄指针
 * @param servo_id 舵机ID（1-250）
 * @retval uint8_t 0=校准成功，1=读取角度失败，2=设零点失败，3=保存失败
 */
uint8_t servo_calibrate_zero(UART_HandleTypeDef *huart, uint8_t servo_id) {
    uint8_t rx_buf[10] = {0};
    uint32_t timeout = 100;
    uint32_t start_tick;

    // 步骤1：读取当前角度（确保舵机在线且角度有效）
    read_servo_angle(huart, servo_id);
    
    // 步骤2：释放扭矩（零点设置需在释放状态，协议3.4.1.2）
    set_servo_torque(huart, servo_id, TORQUE_RELEASE);
    HAL_Delay(ZERO_CALIB_DELAY); // 等待扭矩释放完成

    // 步骤3：发送“设当前位置为零点”指令（协议3.2.4.2 例子3：0x13指令+0xA5A5）
    uint8_t set_zero_frame[7] = {
        0xAB,               // 帧头（主机）
        servo_id,           // 舵机ID
        0x07,               // 长度（帧头+ID+长度+命令+数据+校验 = 7字节）
        0x13,               // 命令（角度调整）
        (SET_ZERO_DATA >> 0) & 0xFF,  // 数据低字节（0xA5）
        (SET_ZERO_DATA >> 8) & 0xFF,  // 数据高字节（0xA5）
        0x00                // 校验位（待计算）
    };
    set_zero_frame[6] = calculate_checksum(set_zero_frame, 7); // 计算校验和
    HAL_UART_Transmit(huart, set_zero_frame, 7, HAL_MAX_DELAY);

    // 步骤4：保存零点配置（掉电不丢失，协议3.2.11）
    save_servo_config(huart, servo_id);
   
    // 步骤5：恢复扭矩（锁舵，避免舵机松动）
    set_servo_torque(huart, servo_id, TORQUE_LOCK);
    HAL_Delay(ZERO_CALIB_DELAY);
    printf("Servo ID%d: Zero Calibration Complete!\r\n", servo_id);
    return 0; // 校准成功
}
/**
 * @brief 所有舵机批量零点校准（开机初始化时调用）
 * @param huart UART句柄指针
 * @param servos 舵机结构体数组
 * @param servo_count 舵机数量
 * @retval uint8_t 0=全部校准成功，1=部分/全部失败
 */
uint8_t servo_batch_calibrate_zero(UART_HandleTypeDef *huart, Servo *servos, uint8_t servo_count) {
    uint8_t calib_result = 0; // 0=全部成功，1=有失败
    printf("\n==================== Servo Zero Calibration Start ====================\r\n");

    for (uint8_t i = 0; i < servo_count; i++) {
        uint8_t id = servos[i].id;
        printf("\nCalibrating Servo ID%d...\r\n", id);
        uint8_t result = servo_calibrate_zero(huart, id);

        // 处理校准结果
        switch (result) {
            case 0:
                printf("Servo ID%d: Calibration Success\r\n", id);
                break;
            case 1:
                printf("Servo ID%d: Calibration Failed (Read Angle Error)\r\n", id);
                calib_result = 1;
                break;
            case 2:
                printf("Servo ID%d: Calibration Failed (Set Zero Error)\r\n", id);
                calib_result = 1;
                break;
            case 3:
                printf("Servo ID%d: Calibration Failed (Save Config Error)\r\n", id);
                calib_result = 1;
                break;
            default:
                printf("Servo ID%d: Calibration Failed (Unknown Error)\r\n", id);
                calib_result = 1;
                break;
        }
        HAL_Delay(300); // 间隔300ms，避免指令冲突
    }

    printf("\n==================== Servo Zero Calibration End ======================\r\n");
    return calib_result;
}
/**
 * @brief 计算校验和(双重字节累加和)
 * @param data 要计算的数据数组
 * @param length 数据长度
 * @retval 计算出的校验和
 */
uint8_t calculate_checksum(uint8_t *data, uint16_t length) {
    uint8_t cks1 = 0xAB;
    uint8_t cks2 = 0xAC;
    
    for(uint16_t i = 0; i < length - 1; i++) {
        cks1 += data[i];
        cks2 += cks1;
    }
    g_checksum=(uint8_t)(cks1 + cks2);
		// 更新最后一位校验值
    data[length - 1] = cks1 + cks2;
    return (uint8_t)(cks1 + cks2);
}
void factory_setting(UART_HandleTypeDef *huart, uint8_t servo_id) {
   
    uint8_t tx_frame[9] = {0xAB, servo_id, 0x09, 0x2F, 0xF1, 0xF2, 0xF3, 0xF4};
    
    // 计算校验和并添加到命令末尾
    tx_frame[8] = calculate_checksum(tx_frame, 9);
    uint8_t tx_frame_len=sizeof(tx_frame)/sizeof(tx_frame[0]);
    // 发送命令
    HAL_UART_Transmit(huart, tx_frame, tx_frame_len, HAL_MAX_DELAY);
    HAL_Delay(100);
		// 第二步: 保持参数，防止掉电丢失
		save_servo_config(huart, servo_id);
}
/**
 * @brief 使用超级ID设置舵机ID
 * @param huart UART句柄指针
 * @param newID 新的舵机ID (1-250)
 * @retval 无
 */
void set_servo_id(UART_HandleTypeDef *huart,uint8_t newID) {
		
    // 第一步: 发送修改ID命令
    int setID_array_len=sizeof(setID_array)/sizeof(setID_array[0]);
		setID_array[4]=newID;
    setID_array[setID_array_len-1]=calculate_checksum(setID_array, setID_array_len);
		HAL_UART_Transmit(huart, setID_array, setID_array_len, HAL_MAX_DELAY);
		HAL_Delay(100);
		// 第二步: 保持参数，防止掉电丢失
		save_servo_config(huart, newID);
}

/**
 * @brief 堵转判断
 * @param huart UART句柄指针
 * @param servo_id 舵机ID
 * @retval 无
 */
void locked_rotor_protection(UART_HandleTypeDef *huart, uint8_t servo_id) {

    uint8_t tx_frame[5] = {0xAB, servo_id, 0x05, 0x30};
    
    // 计算校验和并添加到命令末尾
    tx_frame[4] = calculate_checksum(tx_frame, 5);
    uint8_t tx_frame_len=sizeof(tx_frame)/sizeof(tx_frame[0]);
    // 发送命令
    HAL_UART_Transmit(huart, tx_frame, tx_frame_len, HAL_MAX_DELAY);
		
}

/**
 * @brief 保存舵机配置（防止掉电丢失）
 * @param huart UART句柄指针
 * @param servo_id 舵机ID
 * @retval 无
 */
void save_servo_config(UART_HandleTypeDef *huart, uint8_t servo_id) {
		uint8_t save_tx_frame[9] = {0xAB, 0xFD, 0x09, 0x2E, 0xF1, 0xF2, 0xF3, 0xF4};
		save_tx_frame[8] = calculate_checksum(save_tx_frame, 9); //更新校验位
		HAL_UART_Transmit(huart, save_tx_frame, 9, HAL_MAX_DELAY);
		HAL_Delay(100);
}

/**
 * @brief 设置舵机角度限制
 * @param huart UART句柄指针
 * @param servo_id 舵机ID
 * @param ccw_limit 逆时针角度限制（单位：0.1°）
 * @param cw_limit 顺时针角度限制（单位：0.1°）
 * @retval 无
 */
void set_servo_angle_limits(UART_HandleTypeDef *huart, uint8_t servo_id, 
                           int32_t ccw_limit, int32_t cw_limit) {
		// 如果输入是负数，转换为无符号数
    if (ccw_limit < 0) {
        ccw_limit = 65536 + ccw_limit; // 计算补码
    }
    // 单位: 0.1°, 命令0x14
    // 命令帧: 帧头(0xAB) + ID + 长度(0x0D) + 命令(0x14) + CCW限制(4字节) + CW限制(4字节)
    uint8_t tx_frame[13] = {0xAB, servo_id, 0x0D, 0x14,
                      (uint8_t)(ccw_limit & 0xFF), (uint8_t)((ccw_limit >> 8) & 0xFF),
                      (uint8_t)((ccw_limit >> 16) & 0xFF), (uint8_t)((ccw_limit >> 24) & 0xFF),
                      (uint8_t)(cw_limit & 0xFF), (uint8_t)((cw_limit >> 8) & 0xFF),
                      (uint8_t)((cw_limit >> 16) & 0xFF), (uint8_t)((cw_limit >> 24) & 0xFF)};
    
    // 计算校验和并添加到命令末尾
    tx_frame[12] = calculate_checksum(tx_frame, 13);
    uint8_t tx_frame_len=sizeof(tx_frame)/sizeof(tx_frame)[0];
    // 发送命令
    HAL_UART_Transmit(huart, tx_frame, tx_frame_len, HAL_MAX_DELAY);
    HAL_Delay(10);
		save_servo_config(huart, servo_id);
}
/**
 * @brief 读取舵机当前角度
 * @param huart UART句柄指针
 * @param servo_id 舵机ID
 * @retval 无
 */
void read_servo_angle(UART_HandleTypeDef *huart, uint8_t servo_id) {
		if (servo_id < 1 || servo_id > SERVO_COUNT) {
        printf("read_servo_angle: Invalid servo ID %d\r\n", servo_id);
        return;
    }
    // 命令帧: 帧头(0xAB) + ID + 长度(0x05) + 命令(0x51)
    uint8_t tx_frame[5] = {0xAB, servo_id, 0x05, 0x51};
    
    // 计算校验和并添加到命令末尾
    tx_frame[4] = calculate_checksum(tx_frame, 5);
    uint8_t tx_frame_len=sizeof(tx_frame)/sizeof(tx_frame[0]);
		posframe_checksum = tx_frame_len;
    // 发送命令
    HAL_UART_Transmit(huart, tx_frame, tx_frame_len, HAL_MAX_DELAY);
    HAL_Delay(10);
}
/**
 * @brief 查询舵机当前电流
 * @param huart UART句柄指针
 * @param servo_id 舵机ID
 * @retval 无
 */

void read_servo_current(UART_HandleTypeDef *huart, uint8_t servo_id) {
		if (servo_id < 1 || servo_id > SERVO_COUNT) {
        printf("read_servo_current: Invalid servo ID %d\r\n", servo_id);
        return;
    }
    // 命令帧: 帧头(0xAB) + ID + 长度(0x05) + 命令(0x53)
    uint8_t tx_frame[5] = {0xAB, servo_id, 0x05, 0x53};
    
    // 计算校验和并添加到命令末尾
    tx_frame[4] = calculate_checksum(tx_frame, 5);
    uint8_t tx_frame_len=sizeof(tx_frame)/sizeof(tx_frame[0]);
    // 发送命令
    HAL_UART_Transmit(huart, tx_frame, tx_frame_len, HAL_MAX_DELAY);
    HAL_Delay(10);
}
/**
 * @brief 设置舵机扭矩状态
 * @param huart UART句柄指针
 * @param servo_id 舵机ID
 * @param state 扭矩状态：
 *              0=无阻尼关扭矩
 *              1=有阻尼关扭矩
 *              2=扭矩预开启
 *              3=打开扭矩并在当前位置锁舵
 * @retval 无
 */
void set_servo_torque(UART_HandleTypeDef *huart, uint8_t servo_id, uint8_t state) {
    // 状态: 0=无阻尼关扭矩, 1=有阻尼关扭矩, 2=扭矩预开启, 3=打开扭矩
    // 命令帧: 帧头(0xAB) + ID + 长度(0x06) + 命令(0x40) + 状态
    uint8_t tx_frame[6] = {0xAB, servo_id, 0x06, 0x40, state};
    
    // 计算校验和并添加到命令末尾
    tx_frame[5] = calculate_checksum(tx_frame, 6);
    uint8_t tx_frame_len=sizeof(tx_frame)/sizeof(tx_frame[0]);
    // 发送命令
    HAL_UART_Transmit(huart, tx_frame, tx_frame_len, HAL_MAX_DELAY);
    HAL_Delay(10);
}

/**
 * @brief 设置舵机角度调整参数
 * @param huart UART句柄指针
 * @param servo_id 舵机ID
 * @retval 无
 */
void servo_angle_modify(UART_HandleTypeDef *huart, uint8_t servo_id) {
    // 命令帧: 帧头(0xAB) + ID + 长度(0x0B) + 命令(0x13)
    uint8_t tx_frame[11] = {0xAB, servo_id, 0x0B, 0x13, 0x64, 0x00, 0x01, 0x00, 0x01, 0x00};
    
    // 计算校验和并添加到命令末尾
    tx_frame[10] = calculate_checksum(tx_frame, 11);
    uint8_t tx_frame_len=sizeof(tx_frame)/sizeof(tx_frame[0]);
    // 发送命令
    HAL_UART_Transmit(huart, tx_frame, tx_frame_len, HAL_MAX_DELAY);
    HAL_Delay(10);
		save_servo_config(huart, servo_id);
}


// 添加紧急停止函数
void emergency_stop_servo(UART_HandleTypeDef *huart, uint8_t servo_id) {
    set_servo_torque(huart, servo_id, 0); // 无阻尼关闭扭矩
    printf("[EMERGENCY] Stopped servo ID: %d\n", servo_id);
}

// 添加周期性状态检测函数
void check_servo_status(UART_HandleTypeDef *huart) {
    static uint32_t last_check = 0;
    uint32_t current_time = HAL_GetTick();
    
    // 按固定间隔检测
    if (current_time - last_check >= EMERGENCY_CHECK_INTERVAL) {
        last_check = current_time;
        
        // 轮流查询每个舵机状态
        static uint8_t current_servo = 0;
        uint8_t servo_id = servoIDs[current_servo];
        
        // 发送状态查询命令 (0x50)
        uint8_t tx_frame[5] = {0xAB, servo_id, 0x05, 0x50};
        tx_frame[4] = calculate_checksum(tx_frame, 5);
        HAL_UART_Transmit(huart, tx_frame, 5, HAL_MAX_DELAY);
				HAL_Delay(10);
        
        // 发送温度查询命令 (0x58马达温度)
        uint8_t temp_frame[5] = {0xAB, servo_id, 0x05, 0x58};
        temp_frame[4] = calculate_checksum(temp_frame, 5);
        HAL_UART_Transmit(huart, temp_frame, 5, HAL_MAX_DELAY);
        // 检查紧急标志并处理
        if (emergency_flags[current_servo]) {
            emergency_stop_servo(huart, servo_id);
            emergency_flags[current_servo] = 0; // 清除标志
        }
        
        // 更新下一个待查询舵机
        current_servo = (current_servo + 1) % SERVO_COUNT;
    }
}

/**
 * @brief 设置舵机转动角度和转动速度
 * @param huart UART句柄指针
 * @param servo_id 舵机ID
 * @param angle 目标角度（单位：度）
 * @param speed 转动速度（单位：°/S）
 * @param array 指令帧数组
 * @retval 无
 */
void servo_angle_speed(UART_HandleTypeDef *huart, uint8_t servo_id, int16_t angle, int16_t speed, uint8_t array[ARRAY_SIZE]) {
    // 如果输入是负数，转换为无符号数
    if (angle < 0) {
        angle = 65536 + angle; // 计算补码
    }
		//{0xAB, 0x00, 0x09, 0x42, 0x00, 0x00, 0x00, 0x01};
		// 计算目标角度（单位：0.1°）
    int16_t target_angle = angle * 10;
		array[1]=servo_id;
		array[4]=(uint8_t)(target_angle & 0xFF);
		array[5]=(uint8_t)((target_angle >> 8) & 0xFF);
		array[6]=(uint8_t)(speed & 0xFF);
		array[7]=(uint8_t)((speed >> 8) & 0xFF);
    // 计算校验和并添加到命令末尾
    array[8] = calculate_checksum(array, 9);
	
    // 发送命令
    HAL_UART_Transmit(huart, array, 9, HAL_MAX_DELAY);
    HAL_Delay(10);  //添加10ms
		
		/*// 发送状态查询命令
    uint8_t tx_frame[5] = {0xAB, servo_id, 0x05, 0x50};
    tx_frame[4] = calculate_checksum(tx_frame, 5);
    HAL_UART_Transmit(huart, tx_frame, 5, HAL_MAX_DELAY);
		
		// 检查是否已触发紧急停止
    if (emergency_flags[servo_id - 1]) {
        emergency_stop_servo(huart, servo_id);
        emergency_flags[servo_id - 1] = 0;
    }*/
		
}

/**
 * @brief 设置舵机转动角度和转动速度（电机模式）
 * @param huart UART句柄指针
 * @param servo_id 舵机ID
 * @param angle 目标角度（单位：度）
 * @param speed 转动速度（单位：°/S）
 * @param array 指令帧数组
 * @retval 无
 */
void servo_angle_speed_Motor_mode(UART_HandleTypeDef *huart, uint8_t servo_id, int16_t angle, int16_t speed) {
    // 如果输入是负数，转换为无符号数
    if (angle < 0) {
        angle = 65536 + angle; // 计算补码
    }
		
		// 计算目标角度（单位：0.1°）
    int16_t target_angle = angle * 10;
		
		uint8_t tx_frame[9] = {0xAB, servo_id, 0x09, 0x44, (uint8_t)(target_angle & 0xFF), (uint8_t)((target_angle >> 8) & 0xFF),(uint8_t)(speed & 0xFF),(uint8_t)((speed >> 8) & 0xFF)};
		
    // 计算校验和并添加到命令末尾
    tx_frame[8] = calculate_checksum(tx_frame, 9);
	
    // 发送命令
    HAL_UART_Transmit(huart, tx_frame, 9, HAL_MAX_DELAY);
    HAL_Delay(10);  //添加10ms
		
		/*// 发送状态查询命令
    uint8_t tx_frame[5] = {0xAB, servo_id, 0x05, 0x50};
    tx_frame[4] = calculate_checksum(tx_frame, 5);
    HAL_UART_Transmit(huart, tx_frame, 5, HAL_MAX_DELAY);
		
		// 检查是否已触发紧急停止
    if (emergency_flags[servo_id - 1]) {
        emergency_stop_servo(huart, servo_id);
        emergency_flags[servo_id - 1] = 0;
    }*/
		
}
/**
 * @brief 设置舵机角度和旋转时间
 * @param huart UART句柄指针
 * @param servo_id 舵机ID
 * @param angle 目标角度（单位：度）
 * @param time 旋转时间（单位：毫秒）
 * @param array 指令帧数组
 * @retval 无
 */
void servo_angle_time(UART_HandleTypeDef *huart, uint8_t servo_id, int16_t angle, int16_t time, uint8_t array[ARRAY_SIZE]) {
    // 如果输入是负数，转换为无符号数
    if (angle < 0) {
        angle = 65536 + angle; // 计算补码
    }
		//{0xAB, 0x00, 0x09, 0x41, 0x00, 0x00, 0x00, 0x01};
		// 计算目标角度（单位：0.1°）
    int32_t target_angle = angle * 10;
		array[1]=servo_id;
		array[4]=(uint8_t)(target_angle & 0xFF);
		array[5]=(uint8_t)((target_angle >> 8) & 0xFF);
		array[6]=(uint8_t)(time & 0xFF);
		array[7]=(uint8_t)((time >> 8) & 0xFF);
    // 计算校验和并添加到命令末尾
    array[8] = calculate_checksum(array, 9);
		// 调试输出（可选）
		/*printf("array[0]=0x%02X\r\n", array[0]);  
    printf("array[1]=0x%02X\r\n", array[1]);  
		printf("array[2]=0x%02X\r\n", array[2]);  
    printf("array[3]=0x%02X\r\n", array[3]);  
    printf("array[4]=0x%02X\r\n", array[4]);  
    printf("array[5]=0x%02X\r\n", array[5]);  
    printf("array[6]=0x%02X\r\n", array[6]);  
    printf("array[7]=0x%02X\r\n", array[7]);  
		printf("checksum=0x%02X\r\n", array[8]);*/
    // 发送命令
    HAL_UART_Transmit(huart, array, 9, HAL_MAX_DELAY);
    HAL_Delay(10);
}

/**
 * @brief 设置舵机波特率
 * @param huart UART句柄指针
 * @param servo_id 舵机ID
 * @param baud 波特率值（如115200）
 * @retval 无
 */
void set_servo_baudrate(UART_HandleTypeDef *huart, uint8_t servo_id, uint32_t baud) {
    // 计算波特率值 (实际波特率/100)
    uint16_t baud_value = baud / 100;
		// 命令帧: 帧头(0xAB) + ID + 长度(0x0B) + 命令(0x13)
    uint8_t tx_frame[7] = {0xAB, servo_id, 0x07, 0x11, (uint8_t)(baud_value & 0xFF), (uint8_t)(baud_value >> 8) & 0xFF};
    
    // 计算校验和并添加到命令末尾
    tx_frame[6] = calculate_checksum(tx_frame, 7);
    uint8_t tx_frame_len=sizeof(tx_frame)/sizeof(tx_frame[0]);
    // 发送命令
    HAL_UART_Transmit(huart, tx_frame, tx_frame_len, HAL_MAX_DELAY);
    HAL_Delay(10);
		save_servo_config(huart, servo_id);
}
/**
 * @brief 修改舵机的最大转动速度
 * @param huart UART句柄指针
 * @param servo_id 舵机ID
 * @param maxspeed 最大转动速度（单位：°/S）
 * @retval 无
 */
void set_servo_maxspeed(UART_HandleTypeDef *huart, uint8_t servo_id, uint16_t maxspeed) {
		// 命令帧: 帧头(0xAB) + ID + 长度(0x07) + 命令(0x16)
    uint8_t tx_frame[7] = {0xAB, servo_id, 0x07, 0x16, (uint8_t)(maxspeed & 0xFF), (uint8_t)(maxspeed >> 8) & 0xFF};
    
    // 计算校验和并添加到命令末尾
    tx_frame[6] = calculate_checksum(tx_frame, 7);
    uint8_t tx_frame_len=sizeof(tx_frame)/sizeof(tx_frame[0]);
    // 发送命令
    HAL_UART_Transmit(huart, tx_frame, tx_frame_len, HAL_MAX_DELAY);
    HAL_Delay(10);
		save_servo_config(huart, servo_id);
}
/**
 * @brief 修改舵机的最大PWM输出
 * @param huart UART句柄指针
 * @param servo_id 舵机ID
 * @param maxPWM 最大PWM输出值（范围100-1000，对应10%-100%）
 * @retval 无
 */
void set_servo_maxPWM(UART_HandleTypeDef *huart, uint8_t servo_id, uint16_t maxPWM) {
		// 命令帧: 帧头(0xAB) + ID + 长度(0x07) + 命令(0x18)
    uint8_t tx_frame[7] = {0xAB, servo_id, 0x07, 0x18, (uint8_t)(maxPWM & 0xFF), (uint8_t)(maxPWM >> 8) & 0xFF};
    
    // 计算校验和并添加到命令末尾
    tx_frame[6] = calculate_checksum(tx_frame, 7);
    uint8_t tx_frame_len=sizeof(tx_frame)/sizeof(tx_frame[0]);
    // 发送命令
    HAL_UART_Transmit(huart, tx_frame, tx_frame_len, HAL_MAX_DELAY);
    HAL_Delay(10);
		save_servo_config(huart, servo_id);
}
/**
 * @brief 设置舵机控制精度
 * @param huart UART句柄指针
 * @param servo_id 舵机ID (1-250)
 * @param precision_0_1deg 控制精度（单位：0.1度）
 *        例如：0.1° = 1, 0.5° = 5, 1.0° = 10
 * @retval 无
 */
void set_servo_accuracy(UART_HandleTypeDef *huart, uint8_t servo_id, uint16_t precision_0_1deg) {
		// 命令帧: 帧头(0xAB) + ID + 长度(0x07) + 命令(0x19)
    uint8_t tx_frame[7] = {0xAB, servo_id, 0x07, 0x19, (uint8_t)(precision_0_1deg  & 0xFF), (uint8_t)(precision_0_1deg  >> 8) & 0xFF};
    
    // 计算校验和并添加到命令末尾
    tx_frame[6] = calculate_checksum(tx_frame, 7);
    uint8_t tx_frame_len=sizeof(tx_frame)/sizeof(tx_frame[0]);
		/*printf("tx_frame[0]=0x%02X\r\n", tx_frame[0]);  
    printf("tx_frame[1]=0x%02X\r\n", tx_frame[1]);  
		printf("tx_frame[2]=0x%02X\r\n", tx_frame[2]);  
    printf("tx_frame[3]=0x%02X\r\n", tx_frame[3]);  
    printf("tx_frame[4]=0x%02X\r\n", tx_frame[4]);  
    printf("tx_frame[5]=0x%02X\r\n", tx_frame[5]); 
		printf("checksum=0x%02X\r\n", tx_frame[6]);*/
    // 发送命令
    HAL_UART_Transmit(huart, tx_frame, tx_frame_len, HAL_MAX_DELAY);
    HAL_Delay(10);
		save_servo_config(huart, servo_id);

}
/**
 * @brief 查询舵机出厂ID值
 * @param huart UART句柄指针
 * @retval 无
 */
void inquire_factory_ID(UART_HandleTypeDef *huart) {
		// 命令帧: 帧头(0xAB) + 0xFD + 长度(0x05) + 命令(0x16)
    uint8_t tx_frame[5] = {0xAB, 0xFD, 0x05, 0x03};
    
    // 计算校验和并添加到命令末尾
    tx_frame[4] = calculate_checksum(tx_frame, 5);
    uint8_t tx_frame_len=sizeof(tx_frame)/sizeof(tx_frame[0]);
    // 发送命令
    HAL_UART_Transmit(huart, tx_frame, tx_frame_len, HAL_MAX_DELAY);
    HAL_Delay(10);

}
/**
 * @brief 设置舵机模式
 * @param huart UART句柄指针
 * @param servo_id 舵机ID
 * @param position_mode 位置模式：
 *        0=绝对位置转动（默认）
 *        1=相对当前目标位置转动
 *        2=相对当前实际位置转动
 * @param torque_off_time 扭矩关闭时间（单位0.1秒）：
 *        0=到达目标位置后锁舵（默认）
 *        1-255=到达目标位置后延迟关闭扭矩
 * @retval 无
 */
void set_servo_mode(UART_HandleTypeDef *huart, uint8_t servo_id, uint8_t position_mode, uint8_t torque_off_time) {
		// 命令帧: 帧头(0xAB) + ID + 长度(0x07) + 命令(0x47)
    uint8_t tx_frame[7] = {0xAB, servo_id, 0x07, 0x47, position_mode, torque_off_time};
    
    // 计算校验和并添加到命令末尾
    tx_frame[6] = calculate_checksum(tx_frame, 7);
    uint8_t tx_frame_len=sizeof(tx_frame)/sizeof(tx_frame[0]);
		/*printf("tx_frame[0]=0x%02X\r\n", tx_frame[0]);  
    printf("tx_frame[1]=0x%02X\r\n", tx_frame[1]);  
		printf("tx_frame[2]=0x%02X\r\n", tx_frame[2]);  
    printf("tx_frame[3]=0x%02X\r\n", tx_frame[3]);  
    printf("tx_frame[4]=0x%02X\r\n", tx_frame[4]);  
    printf("tx_frame[5]=0x%02X\r\n", tx_frame[5]); 
		printf("checksum=0x%02X\r\n", tx_frame[6]);*/
    // 发送命令
    HAL_UART_Transmit(huart, tx_frame, tx_frame_len, HAL_MAX_DELAY);
    HAL_Delay(10);
		save_servo_config(huart, servo_id);
}

/**
 * @brief 设置舵机应答延迟时间
 * @param huart UART句柄指针
 * @param servo_id 舵机ID (1-250)
 * @param delay_50us 延迟时间（单位：50微秒）
 *        例如：50us = 1, 100us = 2, 最大65535*50us = 3.277秒
 * @retval 无
 */
void set_response_delay(UART_HandleTypeDef *huart, uint8_t servo_id, uint16_t delay_50us) {
    // 构造发送帧（不含校验位）
    uint8_t tx_frame[7] = {0xAB, servo_id, 0x07, 0x15, (uint8_t)(delay_50us & 0xFF), (uint8_t)(delay_50us >> 8)};
		// 计算校验和并添加到命令末尾
    tx_frame[6] = calculate_checksum(tx_frame, 7);
    uint8_t tx_frame_len=sizeof(tx_frame)/sizeof(tx_frame[0]);
    // 发送命令
    HAL_UART_Transmit(huart, tx_frame, tx_frame_len, HAL_MAX_DELAY);
    HAL_Delay(10);
		save_servo_config(huart, servo_id);
}
/**
 * @brief 初始化所有舵机
 * @param huart UART句柄指针
 * @retval 无
 */
void init_all_servos(UART_HandleTypeDef *huart) {
    
    // 设置目标舵机的角度限制
		set_servo_angle_limits(&huart5, 1, -45, 45); //腰部
		set_servo_angle_limits(&huart5, 2, -180, 180); //左手
		set_servo_angle_limits(&huart5, 3, -180, 180); //右手
		set_servo_angle_limits(&huart5, 4, -45, 45); //头左右
		set_servo_angle_limits(&huart5, 5, -22, 22); //头上下
		set_servo_angle_limits(&huart5, 6, -12, 12); //头摆动 
    // 步骤2: 为每个舵机分配唯一ID
    /*for(int i = 0; i < 6; i++) {
        // 假设所有舵机初始ID为1
        //set_servo_id(huart, more_servos[i].id);//已经单独设置好了
        
        //set_servo_mode(&huart5, more_servos[i].id, 0, 10);// 绝对位置转动，到达目标位置1秒后关闭关闭扭矩，单位0.1S
				//set_response_delay(&huart5, more_servos[i].id, 1);// 应答延迟时间为50us，设置为100us则填2，100us / 50us = 2
				set_servo_maxPWM(&huart5, more_servos[i].id, 100);// PWM为1000
				set_servo_accuracy(&huart5, more_servos[i].id, 1);// 控制精度为0.1°
				
				// 设置目标舵机的最大转动速度
				set_servo_maxspeed(&huart5, more_servos[i].id, 100);

    }	*/
		
		// 新增：开机零点校准（可通过宏定义注释关闭，仅首次安装时启用）
    #define ENABLE_POWER_ON_ZERO_CALIB 0 // 1=启用，0=禁用
    #if ENABLE_POWER_ON_ZERO_CALIB
        printf("\n=== Power-On Servo Zero Calibration (Enable) ===\r\n");
        servo_batch_calibrate_zero(huart, more_servos, 6); // 6个舵机批量校准
				//servo_calibrate_zero(huart, 1);
    #else
        printf("\n=== Power-On Servo Zero Calibration (Disable) ===\r\n");
    #endif
}

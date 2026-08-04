#include "includes.h"
#include "app.h"

uint8_t angle_time_single[ARRAY_SIZE]={0xAB, 0x00, 0x09, 0x41, 0x00, 0x00, 0x00, 0x01};//修改舵机ID为0的为目标角度和转动时间的写指令帧；
uint8_t angle_speed_single[ARRAY_SIZE]={0xAB, 0x00, 0x09, 0x42, 0x00, 0x00, 0x01, 0x00};//修改舵机ID为0的为目标角度和转动速度的写指令帧；
uint8_t Current_threshold_array[ARRAY_SIZE]={0xAB,0x03,0x09,0x32,0x02,0x00,0x01,0x00};//修改舵机ID为3的电流保护阈值为1A，检测时间为1s的写指令帧；
uint8_t setID_array[7]={0xAB, 0xFD, 0x07, 0x10, 0x00, 0x00};//修改舵机ID为的写指令帧
uint8_t servoIDs[SERVO_COUNT] = {1, 2, 3, 4, 5, 6};
uint8_t save_tx_frame[9] = {0xAB, 0xFD, 0x09, 0x2E, 0xF1, 0xF2, 0xF3, 0xF4};

uint8_t g_checksum;
int16_t position;
uint8_t posframe_checksum;

uint8_t angle_speed[6][9]={{0xAB, 0x00, 0x09, 0x42, 0x00, 0x00, 0x01, 0x00},{0xAB, 0x00, 0x09, 0x42, 0x00, 0x00, 0x01, 0x00},{0xAB, 0x00, 0x09, 0x42, 0x00, 0x00, 0x01, 0x00},
{0xAB, 0x00, 0x09, 0x42, 0x00, 0x00, 0x01, 0x00},{0xAB, 0x00, 0x09, 0x42, 0x00, 0x00, 0x01, 0x00},{0xAB, 0x00, 0x09, 0x42, 0x00, 0x00, 0x01, 0x00}};


Servo more_servos[6]={{.id=BOTTOM},{.id=LEFT},{.id=RIGHT},{.id=NECK},{.id=HEAD_NOD},{.id=HEAD_SHAKE}};
Servo my_servo = {
        .id = 1,           // 舵机ID
        .min_angle = -1800, // -180° (单位0.1°)
        .max_angle = 1800,  // 180° (单位0.1°)
        .position = 0,
        .speed = 0
};

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
		
		/*angle_speed[Serve_Index][1] = 1;modifyArrayWithDecimal(angle_speed[Serve_Index],-45);  HAL_UART_Transmit(&huart5, angle_speed[Serve_Index], sizeof(angle_speed[Serve_Index]), HAL_MAX_DELAY);
		HAL_Delay(1000);
		modifyArrayWithDecimal(angle_speed[Serve_Index],45);  HAL_UART_Transmit(&huart5, angle_speed[Serve_Index], sizeof(angle_speed[Serve_Index]), HAL_MAX_DELAY);
		HAL_Delay(1000);*/
		/*sweep_action(&huart5, &more_servos[0], -30, 30, 1, 3);
		int32_t positions[] = {0, 45, 90, 45, 0, -45, -90, -45, 0};
		position_action(&huart5, &more_servos[1], positions, sizeof(positions)/sizeof(positions[0]), 150, 500);

		smooth_sweep_action(&huart5, &more_servos[2], -60, 60, 50, 1000, 3000);
		homing_action(&huart5, &more_servos[2], 1);

		sine_wave_action(&huart5, &more_servos[3], 0, 60, 200, 5);

		sweep_action(&huart5, &more_servos[5], -15, 15, 1, 3);*/
	
	servo_angle_speed(&huart5, BOTTOM, 0, 30, angle_speed[BOTTOM-1]);
	HAL_Delay(1000);
	servo_angle_speed(&huart5, BOTTOM, -45, 30, angle_speed[BOTTOM-1]);
	HAL_Delay(1000);
	servo_angle_speed(&huart5, BOTTOM, 45, 30, angle_speed[BOTTOM-1]);
	HAL_Delay(1000);
	servo_angle_speed(&huart5, BOTTOM, 0, 30, angle_speed[BOTTOM-1]);
	HAL_Delay(1000);
	
	servo_angle_speed(&huart5, LEFT, 0, 30, angle_speed[LEFT-1]);
	HAL_Delay(1000);
	servo_angle_speed(&huart5, LEFT, -45, 30, angle_speed[LEFT-1]);
	HAL_Delay(1000);
	servo_angle_speed(&huart5, LEFT, 45, 30, angle_speed[LEFT-1]);
	HAL_Delay(1000);
	servo_angle_speed(&huart5, LEFT, 0, 30, angle_speed[LEFT-1]);
	HAL_Delay(1000);
	
	servo_angle_speed(&huart5, RIGHT, 0, 30, angle_speed[RIGHT-1]);
	HAL_Delay(1000);
	servo_angle_speed(&huart5, RIGHT, -45, 30, angle_speed[RIGHT-1]);
	HAL_Delay(1000);
	servo_angle_speed(&huart5, RIGHT, 45, 30, angle_speed[RIGHT-1]);
	HAL_Delay(1000);
	servo_angle_speed(&huart5, RIGHT, 0, 30, angle_speed[RIGHT-1]);
	HAL_Delay(1000);
	
	servo_angle_speed(&huart5, NECK, 0, 30, angle_speed[NECK-1]);
	HAL_Delay(1000);
	servo_angle_speed(&huart5, NECK, -45, 30, angle_speed[NECK-1]);
	HAL_Delay(1000);
	servo_angle_speed(&huart5, NECK, 45, 30, angle_speed[NECK-1]);
	HAL_Delay(1000);
	servo_angle_speed(&huart5, NECK, 0, 30, angle_speed[NECK-1]);
	HAL_Delay(1000);
	
	servo_angle_speed(&huart5, HEAD_NOD, 0, 30, angle_speed[HEAD_NOD-1]);
	HAL_Delay(1000);
	servo_angle_speed(&huart5, HEAD_NOD, -22, 30, angle_speed[HEAD_NOD-1]);
	HAL_Delay(1000);
	servo_angle_speed(&huart5, HEAD_NOD, 22, 30, angle_speed[HEAD_NOD-1]);
	HAL_Delay(1000);
	servo_angle_speed(&huart5, HEAD_NOD, 0, 30, angle_speed[HEAD_NOD-1]);
	HAL_Delay(1000);
	
	servo_angle_speed(&huart5, HEAD_SHAKE, 0, 30, angle_speed[HEAD_SHAKE-1]);
	HAL_Delay(1000);
	servo_angle_speed(&huart5, HEAD_SHAKE, -12, 30, angle_speed[HEAD_SHAKE-1]);
	HAL_Delay(1000);
	servo_angle_speed(&huart5, HEAD_SHAKE, 12, 30, angle_speed[HEAD_SHAKE-1]);
	HAL_Delay(1000);
	servo_angle_speed(&huart5, HEAD_SHAKE, 0, 30, angle_speed[HEAD_SHAKE-1]);
	HAL_Delay(1000);
	
	/*angle_speed[Serve_Index][1] = SERVO_ID;modifyArrayWithDecimal(angle_speed[Serve_Index],-180);  HAL_UART_Transmit(&huart5, angle_speed[Serve_Index], sizeof(angle_speed[Serve_Index]), HAL_MAX_DELAY);
	HAL_Delay(1000);
  modifyArrayWithDecimal(angle_speed[Serve_Index],180);  HAL_UART_Transmit(&huart5, angle_speed[Serve_Index], sizeof(angle_speed[Serve_Index]), HAL_MAX_DELAY);
	HAL_Delay(1000);*/
	
}
// 解析舵机应答帧
void parse_servo_response(uint8_t *frame, uint8_t length) 
{
    uint8_t id = frame[1];   // 舵机ID
    uint8_t cmd = frame[3];  // 命令字

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
				case 0x40: // 修改扭矩状态
						printf("Torque status set OK\r\n");
            break;
				case 0x41: // 设置角度和转动时间
            printf("set angle and rotate time OK\r\n");
            break;				
				case 0x42: // 设置角度和转动速度
						printf("set angle and rotate speed OK\r\n");
            break;
				case 0x47: // 舵机模式设置
						printf("set mode OK\r\n");
            break;         
        case 0x51: // 当前位置查询应答
            if (length >= 7) {
                position = (frame[5] << 8) | frame[4]; // 小端格式
                printf("Current position: %d\r\n", position / 10); // 单位0.1°
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
 * @brief 保存舵机配置（防止掉电丢失）
 * @param huart UART句柄指针
 * @param servo_id 舵机ID
 * @retval 无
 */
void save_servo_config(UART_HandleTypeDef *huart, uint8_t servo_id) {
		int save_tx_frame_len=sizeof(save_tx_frame)/sizeof(save_tx_frame[0]);
		save_tx_frame[save_tx_frame_len-1] = calculate_checksum(save_tx_frame, save_tx_frame_len); //更新校验位
		HAL_UART_Transmit(huart, save_tx_frame, save_tx_frame_len, HAL_MAX_DELAY);
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
		save_servo_config(huart, servo_id);
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
		array[4]=(int8_t)(target_angle & 0xFF);
		array[5]=(int8_t)((target_angle >> 8) & 0xFF);
		array[6]=(int8_t)(speed & 0xFF);
		array[7]=(int8_t)((speed >> 8) & 0xFF);
    // 计算校验和并添加到命令末尾
    array[8] = calculate_checksum(array, 9);
		printf("array[0]=0x%02X\r\n", array[0]);  
    printf("array[1]=0x%02X\r\n", array[1]);  
		printf("array[2]=0x%02X\r\n", array[2]);  
    printf("array[3]=0x%02X\r\n", array[3]);  
    printf("array[4]=0x%02X\r\n", array[4]);  
    printf("array[5]=0x%02X\r\n", array[5]);  
    printf("array[6]=0x%02X\r\n", array[6]);  
    printf("array[7]=0x%02X\r\n", array[7]);  
		printf("checksum=0x%02X\r\n", array[8]);
    // 发送命令
    HAL_UART_Transmit(huart, array, 9, HAL_MAX_DELAY);
    //HAL_Delay(10);
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
void servo_angle_time(UART_HandleTypeDef *huart, uint8_t servo_id, int32_t angle, int32_t time, uint8_t array[ARRAY_SIZE]) {
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
    // 步骤1: 设置所有舵机使用相同波特率 (115200)
    for(int i = 0; i < 6; i++) {
				set_servo_baudrate(&huart5, more_servos[i].id, 115200);
    }
    // 设置目标舵机的角度限制
		set_servo_angle_limits(&huart5, 1, -45, 45); //腰部
		set_servo_angle_limits(&huart5, 2, -180, 180); //左手
		set_servo_angle_limits(&huart5, 3, -180, 180); //右手
		set_servo_angle_limits(&huart5, 4, -45, 45); //头左右
		set_servo_angle_limits(&huart5, 5, -22, 22); //头上下
		set_servo_angle_limits(&huart5, 6, -12, 12); //头摆动 
    // 步骤2: 为每个舵机分配唯一ID
    for(int i = 0; i < 6; i++) {
        // 假设所有舵机初始ID为1
        //set_servo_id(huart, more_servos[i].id);//已经单独设置好了
        
        //set_servo_mode(&huart5, more_servos[i].id, 0, 10);// 绝对位置转动，到达目标位置1秒后关闭关闭扭矩，单位0.1S
				//set_response_delay(&huart5, more_servos[i].id, 1);// 应答延迟时间为50us，设置为100us则填2，100us / 50us = 2
				set_servo_maxPWM(&huart5, more_servos[i].id, 100);// PWM为1000
				set_servo_accuracy(&huart5, more_servos[i].id, 1);// 控制精度为0.1°
				
				// 设置目标舵机扭矩状态，修改舵机扭矩状态为扭矩预开启	
				set_servo_torque(&huart5, more_servos[i].id, 2);  //1和2都不支持,只支持2和3
				
				// 设置目标舵机的最大转动速度
				set_servo_maxspeed(&huart5, more_servos[i].id, 100);

    }	
}
/**
 * @brief 扫动模式动作 - 舵机在限定范围内来回摆动
 * @param huart UART句柄指针
 * @param servo 舵机结构体指针
 * @param start_angle 起始角度（度）
 * @param end_angle 结束角度（度）
 * @param speed 转动速度（°/S）
 * @param cycles 循环次数
 * @retval 无
 */
void sweep_action(UART_HandleTypeDef *huart, Servo *servo, int32_t start_angle, int32_t end_angle, int32_t speed, uint8_t cycles) {
    printf("Starting sweep action: %d° to %d° at %d°/s for %d cycles\r\n", 
           start_angle, end_angle, speed, cycles);
    
    for(int i = 0; i < cycles; i++) {
        // 转到起始角度
        servo_angle_speed(huart, servo->id, start_angle, speed, angle_speed[servo->id-1]);
        HAL_Delay(1000);
        
        // 转到结束角度
        servo_angle_speed(huart, servo->id, end_angle, speed, angle_speed[servo->id-1]);
        HAL_Delay(1000);
    }
    
    // 回到中间位置
    servo_angle_speed(huart, servo->id, (start_angle + end_angle)/2, speed, angle_speed[servo->id-1]);
    HAL_Delay(500);
}
									
/**
 * @brief 精确位置控制动作 - 移动到特定角度并保持
 * @param huart UART句柄指针
 * @param servo 舵机结构体指针
 * @param positions 目标角度数组
 * @param count 位置数量
 * @param speed 转动速度（°/S）
 * @param hold_time_ms 保持时间（毫秒）
 * @retval 无
 */
void position_action(UART_HandleTypeDef *huart, Servo *servo, int32_t *positions, uint8_t count, int32_t speed, uint16_t hold_time_ms) {
    printf("Starting position action with %d positions\r\n", count);
    
    for(int i = 0; i < count; i++) {
        printf("Moving to position %d: %d°\r\n", i+1, positions[i]);
        
        // 移动到指定位置
        servo_angle_speed(huart, servo->id, positions[i], speed, angle_speed[servo->id-1]);
        
        // 保持位置
        HAL_Delay(hold_time_ms);
    }
}

/**
 * @brief 速度渐变扫描动作 - 平滑改变位置
 * @param huart UART句柄指针
 * @param servo 舵机结构体指针
 * @param start_angle 起始角度（度）
 * @param end_angle 结束角度（度）
 * @param min_speed 最小速度（°/S）
 * @param max_speed 最大速度（°/S）
 * @param duration_ms 总持续时间（毫秒）
 * @retval 无
 */
void smooth_sweep_action(UART_HandleTypeDef *huart, Servo *servo, int32_t start_angle, int32_t end_angle, int32_t min_speed, int32_t max_speed, uint16_t duration_ms) {
    printf("Starting smooth sweep: %d° to %d° over %d ms\r\n", 
           start_angle, end_angle, duration_ms);
    
    const uint16_t steps = 50;
    const uint16_t step_delay = duration_ms / steps;
    float angle_step = (end_angle - start_angle) / (float)steps;
    
    for(int i = 0; i <= steps; i++) {
        // 计算当前角度
        int32_t current_angle = start_angle + (int32_t)(angle_step * i);
        
        // 计算当前速度 (中间快，两端慢)
        float progress = (float)i / steps;
				// 确保定义 M_PI
				#ifndef M_PI
				#define M_PI 3.14159265358979323846
				#endif
        float speed_factor = sinf(progress * M_PI); // 正弦曲线速度变化
        int32_t current_speed = min_speed + (int32_t)((max_speed - min_speed) * speed_factor);
        
        // 设置位置和速度
        servo_angle_speed(huart, servo->id, current_angle, current_speed, angle_speed[servo->id-1]);
        
        HAL_Delay(step_delay);
    }
}
/**
 * @brief 正弦波运动动作 - 模拟自然运动
 * @param huart UART句柄指针
 * @param servo 舵机结构体指针
 * @param center_angle 中心角度（度）
 * @param amplitude 振幅（度）
 * @param speed 转动速度（°/S）
 * @param cycles 循环次数
 * @retval 无
 */
void sine_wave_action(UART_HandleTypeDef *huart, Servo *servo, int32_t center_angle, int32_t amplitude, int32_t speed, uint8_t cycles) {
    printf("Starting sine wave motion: center=%d°, amplitude=%d°, speed=%d°/s for %d cycles\r\n", 
           center_angle, amplitude, speed, cycles);
    
    const uint16_t steps = 36; // 每周期36步 (10°步长)
    const uint16_t step_delay = 1000 / steps; // 控制运动速度
    
    for(int cycle = 0; cycle < cycles; cycle++) {
        for(int i = 0; i < steps; i++) {
            // 计算正弦值 (0-2π)
            float angle_rad = (2 * M_PI * i) / steps;
            
            // 计算目标角度
            int32_t target_angle = center_angle + (int32_t)(amplitude * sinf(angle_rad));
            
            // 设置位置
            servo_angle_speed(huart, servo->id, target_angle, speed, angle_speed[servo->id-1]);
            
            HAL_Delay(step_delay);
        }
    }
}
/**
 * @brief 归位动作 - 缓慢回到零位
 * @param huart UART句柄指针
 * @param servo 舵机结构体指针
 * @param speed 转动速度（°/S）
 * @retval 无
 */
void homing_action(UART_HandleTypeDef *huart, Servo *servo, int32_t speed) {
    printf("Returning to home position (0°) at %d°/s\r\n", speed);
    
    servo_angle_speed(huart, servo->id, 0, speed, angle_speed[servo->id-1]);
    HAL_Delay(1000); // 等待归位完成
}

#ifndef SQ10_H
#define SQ10_H

#include "includes.h"
#include "app.h"
#include <stdbool.h>
// 舵机状态
typedef struct {
    uint8_t id;
   
} Servo;

// 零点设置相关宏定义（基于高德诚舵机协议）
#define SET_ZERO_DATA 0xA5A5    // 设当前位置为中位的专用数据（协议0x13指令）
#define TORQUE_RELEASE 0        // 无阻尼关扭矩（协议0x40指令，释放状态）
#define TORQUE_LOCK 3           // 打开扭矩并锁舵（协议0x40指令，校准后恢复）
#define ZERO_CALIB_DELAY 500     // 校准过程中延迟（确保指令执行完成）

//外部声明变量
extern uint8_t angle_time_single[ARRAY_SIZE];
extern uint8_t angle_speed_single[ARRAY_SIZE];
extern uint8_t Current_threshold_array[ARRAY_SIZE];
extern uint8_t Current_S20_Id;
extern uint8_t servoIDs[SERVO_COUNT];
extern Servo my_servo;
extern Servo more_servos[6];
extern uint8_t angle_speed[6][9];
extern int16_t position;
extern uint8_t posframe_checksum;
extern uint8_t emergency_flags[SERVO_COUNT];
extern uint8_t servo_status[SERVO_COUNT];
extern volatile uint8_t current_servo_index;            // 当前检测的舵机索引
extern volatile uint8_t servo_status_check_flag;
extern int16_t g_servo_current_angle[SERVO_COUNT];
extern bool update_servo[6];
extern uint16_t cur_current;

void modifyArrayWithDecimal(uint8_t array[ARRAY_SIZE], int32_t decimalValue);

void S20_Pro(void);
void updateChecksum(uint8_t array[ARRAY_SIZE]);
uint8_t calculate_checksum(uint8_t *data, uint16_t length);
void factory_setting(UART_HandleTypeDef *huart, uint8_t servo_id);
void set_servo_id(UART_HandleTypeDef *huart,uint8_t newID);
void save_servo_config(UART_HandleTypeDef *huart, uint8_t servo_id);
void set_servo_angle_limits(UART_HandleTypeDef *huart, uint8_t servo_id, int32_t ccw_limit, int32_t cw_limit);
void read_servo_angle(UART_HandleTypeDef *huart, uint8_t servo_id);
void set_servo_torque(UART_HandleTypeDef *huart, uint8_t servo_id, uint8_t state);
void servo_angle_modify(UART_HandleTypeDef *huart, uint8_t servo_id);
void servo_angle_speed(UART_HandleTypeDef *huart, uint8_t servo_id, int16_t angle, int16_t speed, uint8_t array[ARRAY_SIZE]);
void servo_angle_time(UART_HandleTypeDef *huart, uint8_t servo_id, int16_t angle, int16_t time, uint8_t array[ARRAY_SIZE]);
void set_servo_baudrate(UART_HandleTypeDef *huart, uint8_t id, uint32_t baud);
void set_servo_maxspeed(UART_HandleTypeDef *huart, uint8_t servo_id, uint16_t maxspeed);
void set_servo_maxPWM(UART_HandleTypeDef *huart, uint8_t servo_id, uint16_t maxPWM);
void set_servo_accuracy(UART_HandleTypeDef *huart, uint8_t servo_id, uint16_t precision_0_1deg);
void inquire_factory_ID(UART_HandleTypeDef *huart);
void set_servo_mode(UART_HandleTypeDef *huart, uint8_t servo_id, uint8_t position_mode, uint8_t torque_off_time);
void set_response_delay(UART_HandleTypeDef *huart, uint8_t servo_id, uint16_t delay_50us);
void init_all_servos(UART_HandleTypeDef *huart);
void parse_servo_response(uint8_t *frame, uint8_t length);
void emergency_stop_servo(UART_HandleTypeDef *huart, uint8_t servo_id);
void check_servo_status(UART_HandleTypeDef *huart);
uint8_t servo_calibrate_zero(UART_HandleTypeDef *huart, uint8_t servo_id);
uint8_t servo_batch_calibrate_zero(UART_HandleTypeDef *huart, Servo *servos, uint8_t servo_count);
void read_servo_current(UART_HandleTypeDef *huart, uint8_t servo_id);
void servo_angle_speed_Motor_mode(UART_HandleTypeDef *huart, uint8_t servo_id, int16_t angle, int16_t speed);
#endif




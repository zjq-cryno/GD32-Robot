#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <time.h>
#include <stdint.h>
#include <errno.h>
#include <termios.h>
#include <math.h>
#include <pthread.h>

int sock;
struct can_frame frame;
uint8_t system_frame_cnt = 1;

// 计算校验和（异或前7个字节）
uint8_t calc_xor(uint8_t *data) {
    uint8_t res = 0;
    for (int i = 0; i < 7; i++) {
        res ^= data[i];
    }
    return res;
}

// 通用发送帧并等待ACK函数
int send_frame_wait_ack(int sock, uint16_t canid, const uint8_t *data, int data_len) {
    int sent = 0;
    struct can_frame ack = {0};

    while (sent < data_len) {
        memset(&frame, 0, sizeof(frame));
        frame.can_id = canid;

        // 计算本帧数据长度
        int remain = data_len - sent;
        int frame_data_len = (remain >= 5) ? 5 : remain;
        
        // 填充帧数据
        frame.data[0] = frame_data_len;
        memcpy(&frame.data[1], &data[sent], frame_data_len);
        if (frame_data_len < 5) {
            memset(&frame.data[1 + frame_data_len], 0, 5 - frame_data_len);
        }

        frame.data[6] = system_frame_cnt++;
        frame.data[7] = calc_xor(frame.data);
        frame.can_dlc = 8;

        // 发送帧
        printf("Send frame [系统帧计次 %d]: ", frame.data[6]);
        int nbytes = write(sock, &frame, sizeof(frame));
        if (nbytes != sizeof(frame)) {
            perror("CAN send error");
            return -1;
        }
        for (int i = 0; i < 8; i++) printf("%02X ", frame.data[i]);
        printf("\n");

        // 等待ACK
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        struct timeval timeout = {2, 0};
        int ret = select(sock + 1, &readfds, NULL, NULL, &timeout);

        if (ret > 0 && FD_ISSET(sock, &readfds)) {
            int n = read(sock, &ack, sizeof(ack));
            if (n == sizeof(ack) && ack.can_id == canid) {
                uint8_t ack_type = ack.data[0];
                printf("收到ACK: ");
                for (int i = 0; i < 8; i++) printf("%02X ", ack.data[i]);
                printf("\n");
                
                if (ack_type == 0x00) return 0;       // 正确ACK
                else if (ack_type == 0xFF) return -2;  // 错误ACK
            }
        } else {
            printf("超时或错误\n");
            return -1;
        }

        sent += frame_data_len;
    }
    return 0;
}

// 发送指令并等待ACK（适用于0x100/0x101/0x102指令）
int send_cmd_wait_ack(int sock, uint16_t canid, uint8_t servo_id, int16_t angle, uint16_t speed) {
    // 角度处理（补码转换）
    if (angle < 0) angle = 65536 + angle;
    angle *= 10;

    // 准备数据
    uint8_t data[5] = {
        servo_id,
        (uint8_t)(angle & 0xFF),
        (uint8_t)((angle >> 8) & 0xFF),
        (uint8_t)(speed & 0xFF),
        (uint8_t)((speed >> 8) & 0xFF)
    };

    return send_frame_wait_ack(sock, canid, data, 5);
}

// 发送关闭舵机扭矩指令并等待ACK
int send_close_torque_cmd_wait_ack(int sock, uint16_t canid, uint8_t servo_id, uint8_t torque_value) {
    uint8_t data[5] = {servo_id, torque_value, 0x00, 0x00, 0x00};
    return send_frame_wait_ack(sock, canid, data, 5);
}

// 通用查询函数（角度/电流查询通用逻辑）
int send_query_wait_resp(int sock, uint16_t canid, uint8_t servo_id, int16_t *out_value, 
                        const char *resp_desc, int (*parse_resp)(const struct can_frame*, int16_t*)) {
    struct can_frame send = {0}, resp = {0};
    send.can_id = canid;
    send.can_dlc = 8;
    memset(send.data, 0, 8);
    send.data[0] = 1;               // 剩余字节
    send.data[1] = servo_id;
    send.data[6] = system_frame_cnt++;
    send.data[7] = calc_xor(send.data);

    // 发送查询帧
    if (write(sock, &send, sizeof(send)) != sizeof(send)) {
        perror("CAN send error");
        return -1;
    }

    // 等待响应
    time_t start = time(NULL);
    while (time(NULL) - start < 3) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        struct timeval timeout = {0, 400000};  // 400ms超时
        int ret = select(sock + 1, &readfds, NULL, NULL, &timeout);

        if (ret > 0 && FD_ISSET(sock, &readfds)) {
            int n = read(sock, &resp, sizeof(resp));
            if (n == sizeof(resp) && resp.can_id == canid) {
                uint8_t ack_type = resp.data[0];
                if (ack_type == 0x00) {
                    if (parse_resp(&resp, out_value)) {
                        printf("收到%s应答: ", resp_desc);
                        for (int i = 0; i < 8; i++) printf("%02X ", resp.data[i]);
                        printf("\n");
                        return 0;
                    }
                } else if (ack_type == 0xFF) {
                    printf("收到错误应答!\n");
                    return -3;
                }
            }
        }
    }

    printf("未收到%s应答（超时）\n", resp_desc);
    return -4;
}

// 解析角度响应
static int parse_angle_resp(const struct can_frame *resp, int16_t *angle) {
    *angle = ((resp->data[2] << 8) | resp->data[1]) / 10;
    return 1;
}

// 解析电流响应
static int parse_current_resp(const struct can_frame *resp, int16_t *current) {
    *current = ((resp->data[2] << 8) | resp->data[1]) / 10;  // 转换为A
    return 1;
}

// 发送查询舵机位置并等待应答
int send_query_pos_wait_ack(int sock, uint8_t servo_id, int16_t *angle_out) {
    return send_query_wait_resp(sock, 0x103, servo_id, angle_out, 
                              "位置", parse_angle_resp);
}

// 发送查询舵机电流并等待应答
int send_query_current_wait_ack(int sock, uint8_t servo_id, int16_t *current_out) {
    return send_query_wait_resp(sock, 0x105, servo_id, current_out, 
                              "电流", parse_current_resp);
}

int main() {
    struct sockaddr_can addr;
    struct ifreq ifr;

    // 设置CAN接口
    system("sudo ip link set down can0");
    system("sudo ip link set can0 type can bitrate 500000");
    system("sudo modprobe slcan");
    system("sudo ip link set up can0");

    // 打开SocketCAN套接字
    if ((sock = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("socket");
        return 1;
    }

    strcpy(ifr.ifr_name, "can0");
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl");
        close(sock);
        return 1;
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sock);
        return 1;
    }

    // 交互模式
    char input[100];
    printf("请输入命令 (W 舵机号 角度 (速度) | N 舵机号 扭矩值 | R 舵机号 | C 舵机号 | Q退出):\n");
    int servo_id, angle, speed = 50;
    uint8_t torque_value;

    while (1) {
        printf("> ");
        if (!fgets(input, sizeof(input), stdin)) break;
        
        // 去除换行符
        input[strcspn(input, "\n")] = '\0';
        if (strlen(input) == 0) continue;

        // 解析命令
        char cmd[10];
        int parsed = sscanf(input, "%s", cmd);
        if (parsed < 1) continue;

        if ((cmd[0] == 'W' || cmd[0] == 'w')) {
            // 处理W命令（支持带速度和不带速度两种格式）
            int parsed_args = sscanf(input, "%s %d %d %d", cmd, &servo_id, &angle, &speed);
            if (parsed_args >= 3) {
                printf("控制舵机%d 旋转到 %d 度, 速度 %d\n", servo_id, angle, speed);
                int ret = send_cmd_wait_ack(sock, 0x100, servo_id, angle, speed);
                if (ret < 0) printf("控制舵机失败, 错误码: %d\n", ret);
            } else {
                printf("命令格式错误! 正确格式: W 舵机号 角度 (速度)\n");
            }
        }
        else if ((cmd[0] == 'N' || cmd[0] == 'n')) {
            // 处理N命令
            if (sscanf(input, "%s %d %hhd", cmd, &servo_id, &torque_value) == 3) {
                int ret = send_close_torque_cmd_wait_ack(sock, 0x104, servo_id, torque_value);
                if (ret < 0) printf("控制舵机失败, 错误码: %d\n", ret);
            } else {
                printf("命令格式错误! 正确格式: N 舵机号 扭矩值\n");
            }
        }
        else if ((cmd[0] == 'R' || cmd[0] == 'r')) {
            // 处理R命令
            if (sscanf(input, "%s %d", cmd, &servo_id) == 2) {
                printf("读取舵机%d 当前角度\n", servo_id);
                int16_t current_angle;
                int ret = send_query_pos_wait_ack(sock, servo_id, &current_angle);
                if (ret == 0) printf("舵机%d 当前角度: %d\n", servo_id, current_angle);
                else printf("读取舵机角度失败, 错误码: %d\n", ret);
            } else {
                printf("命令格式错误! 正确格式: R 舵机号\n");
            }
        }
        else if ((cmd[0] == 'C' || cmd[0] == 'c')) {
            // 处理C命令
            if (sscanf(input, "%s %d", cmd, &servo_id) == 2) {
                printf("读取舵机%d 当前电流\n", servo_id);
                int16_t current_current;
                int ret = send_query_current_wait_ack(sock, servo_id, &current_current);
                if (ret == 0) printf("舵机%d 当前电流: %d A\n", servo_id, current_current);
                else printf("读取舵机电流失败, 错误码: %d\n", ret);
            } else {
                printf("命令格式错误! 正确格式: C 舵机号\n");
            }
        }
        else if ((cmd[0] == 'Q' || cmd[0] == 'q')) {
            // 退出程序
            break;
        }
        else {
            printf("未知命令! 请使用:\n");
            printf("W 舵机号 角度 (速度) - 控制舵机旋转\n");
            printf("N 舵机号 扭矩值 - 修改舵机扭矩\n");
            printf("R 舵机号 - 读取舵机当前角度\n");
            printf("C 舵机号 - 读取舵机当前电流\n");
            printf("Q - 退出程序\n");
        }
    }
    
    close(sock);
    return 0;
}
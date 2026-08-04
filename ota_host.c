/*******************************************************************
 * @file   : ota_host.c
 * @version: v0.3
 * @author : Jiaqiang.zhou
 * @date   : 2025-08-27
 * @brief  : 
 * ota_host.c - 上位机CAN简易OTA升级测试代码（递归查找bin文件，SocketCAN固定can0，详细注释，自动抓包并自动关闭candump）
 * 编译: gcc ota_host.c -o ota_host -lssl -lcrypto -lcjson -lcurl
 * 用法: ./ota_host
 * 程序自动fork/exec candump到后台，升级结束或异常自动关闭candump，无需手动ctrl+c
 * 新增功能:
 * 1. 从/home/sunrise/ros2_ws/config/robot_config.yaml读取本机设备ID
 * 2. 每分钟API获取bin文件信息，解析API返回的目标设备ID，只对目标设备ID匹配时才升级
 
 *******************************************************************/

/********************************************************************
 *                            INCLUDE                               *
 *******************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <stdint.h>
#include <errno.h>
#include <sys/time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pwd.h>
#include <time.h>
#include <openssl/sha.h>  // 需要链接-lssl -lcrypto
#include <cjson/cJSON.h>  // 需要链接-lcjson
#include <sys/timerfd.h>
#include <poll.h>
#include <curl/curl.h>

#define OTA_CAN_REQUEST_ID 0x401 // OTA升级请求帧ID
#define OTA_CAN_DATA_ID 0x402   // OTA升级数据帧ID
#define MAX_RETRY 10
#define CHECK_INTERVAL_SEC 60 // 定时检测周期，单位秒（如1分钟）
#define DEVICE_CONFIG_PATH      "/home/sunrise/xiaoman/robot_ros2/config/robot_config.yaml"//"/home/sunrise/ros2_ws/config/robot_config.yaml" //
#define DEVICE_ID_MAXLEN 128

// 用于保存GET返回的内容
struct mem_chunk {
    char *memory;
    size_t size;
};
const char *token;
const char *api_token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpZCI6IjYwOWM0ZDAxLWJmMTctNGY5YS1iYWQ2LTk0NzAwZTgyZWVmMCIsInVzZXJuYW1lIjoid211aiIsImlhdCI6MTc1MTQyNjUxMywiZXhwIjoxNzUyMDMxMzEzfQ.InZOJe3OcSG8q7yh_BzT0lTd1Rr4NjIWpFPkX6NRurM";
char local_device_id[DEVICE_ID_MAXLEN] = {0};

int do_ota_upgrade(const char *binfile, int is_single_device, const char *bin_id, const char *token);
int upload_single_device_log(const char *upgradeId, int progress, const char *status, const char *message, const char *token);
/**
 * @brief 读取本机设备ID（从yaml文本文件robot_config.yaml，解析robot: device_id: "xxx"）
 * @param dev_id 输出设备ID字符串
 * @param maxlen 输出缓冲区大小
 * @return 1成功，0失败
 */
int get_local_device_id(char *dev_id, size_t maxlen) {
    FILE *fp = fopen(DEVICE_CONFIG_PATH, "r");
    if (!fp) {
        fprintf(stderr, "无法打开本地设备ID配置文件: %s\n", DEVICE_CONFIG_PATH);
        return 0;
    }
    char line[512];
    int found = 0;
    while (fgets(line, sizeof(line), fp)) {
        char *p = strstr(line, "device_id:");
        if (p) {
            p += strlen("device_id:");
            // 跳过空格和冒号和引号
            while (*p == ' ' || *p == '\t' || *p == '\"') p++;
            // 读取到下一个引号或换行
            char *end = p;
            while (*end && *end != '"' && *end != '\n' && *end != '\r') end++;
            size_t len = end - p;
            if (len >= maxlen) len = maxlen - 1;
            strncpy(dev_id, p, len);
            dev_id[len] = 0;
            found = 1;
            break;
        }
    }
    fclose(fp);
    if (!found) {
        fprintf(stderr, "未在配置文件中找到device_id字段\n");
        return 0;
    }
    return 1;
}

/**
 * @brief 记录OTA升级日志，包括文件名、大小、SHA256、状态
 */
void log_ota_upgrade(const char* binfile, int file_len, const char* status, const uint8_t* file_data) {
    char log_path[1024];
    struct passwd *pw = getpwuid(getuid());
    snprintf(log_path, sizeof(log_path), "%s/ota_upgrade.log", pw->pw_dir);

    FILE *logf = fopen(log_path, "a");
    if (!logf){
        perror("日志文件打开失败");
        return;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    unsigned char sha256[SHA256_DIGEST_LENGTH];
    SHA256(file_data, file_len, sha256);

    char sha256str[SHA256_DIGEST_LENGTH*2+1] = {0};
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        sprintf(sha256str + i*2, "%02x", sha256[i]);

    fprintf(logf, "[%04d-%02d-%02d %02d:%02d:%02d] BIN: %s, Size: %d, SHA256: %s, Status: %s\n",
        t->tm_year+1900, t->tm_mon+1, t->tm_mday,
        t->tm_hour, t->tm_min, t->tm_sec,
        binfile, file_len, sha256str, status);
    printf("log: %s %d %s\n", binfile, file_len, status);
    fclose(logf);
}
// 写入bin文件名和upgradeId的映射，覆盖同名bin的旧记录
void set_upgrade_id_by_filename(const char *filename, const char *upgradeId) {
    struct passwd *pw = getpwuid(getuid());
    char path[1024];
    snprintf(path, sizeof(path), "%s/.last_upgrade_id", pw->pw_dir);

    // 先读入全部内容
    FILE *fp = fopen(path, "r");
    char lines[32][256] = {0};
    int n = 0, replaced = 0;
    if (fp) {
        while (fgets(lines[n], sizeof(lines[n]), fp) && n < 32) {
            char file[128], id[128];
            if (sscanf(lines[n], "%127s %127s", file, id) == 2) {
                if (strcmp(file, filename) == 0) {
                    snprintf(lines[n], sizeof(lines[n]), "%s %s\n", filename, upgradeId);
                    replaced = 1;
                }
            }
            n++;
        }
        fclose(fp);
    }
    // 如果没有被替换，添加一行
    if (!replaced && n < 32) {
        snprintf(lines[n], sizeof(lines[n]), "%s %s\n", filename, upgradeId);
        n++;
    }
    // 写回文件
    fp = fopen(path, "w");
    if (!fp) return;
    for (int i = 0; i < n; i++) fputs(lines[i], fp);
    fclose(fp);
}
// ========== upgradeId本地持久化（多bin映射） ==========
// 读取bin文件名对应的upgradeId，如果找到返回1，未找到返回0
int get_upgrade_id_by_filename(const char *filename, char *upgradeId, size_t len) {
    struct passwd *pw = getpwuid(getuid());
    char path[1024];
    snprintf(path, sizeof(path), "%s/.last_upgrade_id", pw->pw_dir);
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    char line[256];
    int found = 0;
    while (fgets(line, sizeof(line), fp)) {
        char file[128], id[128];
        if (sscanf(line, "%127s %127s", file, id) == 2 && strcmp(file, filename) == 0) {
            strncpy(upgradeId, id, len-1);
            upgradeId[len-1] = 0;
            found = 1;
            break;
        }
    }
    fclose(fp);
    return found;
}
/**
 * @brief 递归查找目录下的第一个.bin文件，找到后返回全路径
 * @return 1找到，0未找到
 */
int find_bin_file(const char *dir, char *found_path, size_t maxlen) {
    DIR *dp = opendir(dir);
    if (!dp) return 0;
    struct dirent *entry;
    struct stat st;
    char path[1024];
    while ((entry = readdir(dp)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
        if (stat(path, &st) < 0) continue;
        if (!S_ISDIR(st.st_mode)) {
            size_t len = strlen(entry->d_name);
            if (len > 4 && strcmp(entry->d_name + len - 4, ".bin") == 0) {
                strncpy(found_path, path, maxlen - 1);
                found_path[maxlen - 1] = 0;
                closedir(dp);
                return 1;
            }
        }
    }
    closedir(dp);
    return 0;
}

/**
 * @brief 计算CAN帧前7字节的异或校验
 */
uint8_t calc_xor(uint8_t *data) {
    uint8_t res = 0;
    for (int i = 0; i < 7; i++) res ^= data[i];
    return res;
}

/**
 * @brief 发送一帧并等待ACK, 若收到NACK(0xFF)则重发。收到ACK(0x00)才返回。支持0~255帧计次循环。
 * @param sock      can0的socket句柄
 * @param canid     CAN帧ID
 * @param data      8字节数据
 * @param frame_cnt 当前帧计次
 * @return 0成功，-1失败
 */
int send_frame_wait_ack(int sock, uint16_t canid, uint8_t *frame, uint8_t frame_cnt) {
    struct can_frame send = {0}, ack = {0};
    memcpy(send.data, frame, 8);
    send.can_id = canid;
    send.can_dlc = 8;
    int retry = 0;
    while (retry < MAX_RETRY) {
        if (write(sock, &send, sizeof(send)) != sizeof(send)) {
            perror("CAN send error");
            return -1;
        }
        // 等待1秒ACK
        fd_set readfds; FD_ZERO(&readfds); FD_SET(sock, &readfds);
        struct timeval timeout = {2, 0};
        int ret = select(sock+1, &readfds, NULL, NULL, &timeout);
        if (ret > 0 && FD_ISSET(sock, &readfds)) {
            int n = read(sock, &ack, sizeof(ack));
            if (n == sizeof(ack) && ack.can_id == canid) {
                uint8_t ack_type = ack.data[0];
                if (ack_type == 0x00) {
                    return 0; // 正确ACK
                } else if (ack_type == 0xFF) {
                    printf("NACK，重发\n");
                    retry++;
                    continue; // NACK重发
                } else if (ack_type == 0xFE) {
                    printf("\nOTA升级数据发送完毕！\n");
                    return 0; // OTA完成
                }
            }
        } else {
            printf("等待ACK超时, 重发\n");
            
        }
        retry++;
    }
    printf("发送失败\n");
    return -1;
}

/**
 * @brief 升级结束清理资源，关闭socket、释放内存、杀candump
 */
void cleanup(int *sock, uint8_t *file_data, pid_t candump_pid) {
    if (file_data) free(file_data);
    if (sock && *sock > 0) close(*sock);
    if (candump_pid > 0) {
        kill(candump_pid, SIGINT);
        waitpid(candump_pid, NULL, 0);
        printf("candump stopped.\n");
    }
}

/**
 * @brief OTA升级进度条打印
 */
void print_progress(int sent, int total) {
    const int bar_width = 20;
    int percent = (int)((sent * 100.0) / total + 0.5);
    int bars = (int)(sent * bar_width / (double)total + 0.5);
    if (bars > bar_width) bars = bar_width;
    printf("\r[");
    for (int i = 0; i < bar_width; i++) {
        if (i < bars) printf("=");
        else if (i == bars && bars < bar_width) printf(">");
        else printf(" ");
    }
    printf("] %3d%% (%d/%d)", percent, sent, total);
    fflush(stdout);
}

static size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct mem_chunk *mem = (struct mem_chunk *)userp;
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL) return 0;
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}



// 查询升级任务接口，返回upgradeId（成功返回1，失败返回0）
int get_upgrade_id_from_backend(char *upgradeId, size_t up_id_len, const char *filename, const char *token) {
    CURL *curl;
    CURLcode res;
    struct mem_chunk chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if(!curl) { free(chunk.memory); return 0; }
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
    curl_easy_setopt(curl, CURLOPT_URL, "https://seeky.cc/api/ota/upgrade-info/all");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    struct curl_slist *headers = NULL;
    char auth_header[512];
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", token);
    headers = curl_slist_append(headers, auth_header);
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if(res != CURLE_OK) { free(chunk.memory); return 0; }
    cJSON *root = cJSON_Parse(chunk.memory);
    int found = 0;
    const char *p1 = strrchr(filename, '/');
    const char *shortname = p1 ? p1 + 1 : filename;
    if (root) {
        cJSON *item = NULL;
        if (cJSON_IsArray(root)) {
            cJSON_ArrayForEach(item, root) {
                cJSON *fn = cJSON_GetObjectItem(item, "filename");
                cJSON *upid = cJSON_GetObjectItem(item, "upgradeId");
                if (fn && upid && strcmp(fn->valuestring, shortname) == 0) {
                    strncpy(upgradeId, upid->valuestring, up_id_len-1);
                    upgradeId[up_id_len-1] = 0;
                    found = 1;
                    set_upgrade_id_by_filename(shortname, upgradeId); // 绑定bin和upgradeId
                    break;
                }
            }
        } else {
            cJSON *fn = cJSON_GetObjectItem(root, "filename");
            cJSON *upid = cJSON_GetObjectItem(root, "upgradeId");
            if (fn && upid && strcmp(fn->valuestring, filename) == 0) {
                strncpy(upgradeId, upid->valuestring, up_id_len-1);
                upgradeId[up_id_len-1] = 0;
                found = 1;
                set_upgrade_id_by_filename(filename, upgradeId);
            }
        }
        cJSON_Delete(root);
    }
    printf("upgradeId: %s\n", upgradeId);
    printf("API返回内容：%s\n", chunk.memory);
    free(chunk.memory);
    return found;
}
// 整合通用日志上传到后台（下载/升级完成）
int upload_ota_log_to_backend(const char *filename,int progress,const char *status,const char *message,const char *token) 
{
    char upgradeId[128] = {0};
    const char *p1 = strrchr(filename, '/');
    const char *shortname = p1 ? p1 + 1 : filename;
    if (!get_upgrade_id_by_filename(shortname, upgradeId, sizeof(upgradeId))) {
        if (!get_upgrade_id_from_backend(upgradeId, sizeof(upgradeId), shortname, token)) {
            printf("获取upgradeId失败，无法上报日志！\n");
            return 0;
        }
    }
    printf("上报日志到后台...(upgradeId=%s, progress=%d, status=%s, message=%s)\n", upgradeId, progress, status, message);

    CURL *curl;
    CURLcode res;
    int success = 0;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        curl_easy_setopt(curl, CURLOPT_URL, "https://seeky.cc/api/ota/upgrade-progress/all");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");

        struct curl_slist *headers = NULL;
        char auth_header[512];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", token);
        headers = curl_slist_append(headers, auth_header);
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        char data[512];
        snprintf(data, sizeof(data),
            "{\n"
            "  \"upgradeId\": \"%s\",\n"
            "  \"progress\": %d,\n"
            "  \"status\": \"%s\",\n"
            "  \"message\": \"%s\"\n"
            "}", upgradeId, progress, status, message);

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            printf("日志上传成功: %s %s\n", status, message);
            success = 1;
            
            // 记录状态到本地文件
            struct passwd *pw = getpwuid(getuid());
            if (strcmp(status, "completed") == 0) {
                // 记录完成状态
                char status_file[1024];
                snprintf(status_file, sizeof(status_file), "%s/.ota_status_%s", pw->pw_dir, upgradeId);
                FILE *sf = fopen(status_file, "w");
                if (sf) {
                    fprintf(sf, "completed\n");
                    fclose(sf);
                }
                
                // 清除下载状态文件
                char download_status_file[1024];
                snprintf(download_status_file, sizeof(download_status_file), 
                    "%s/.%s_download_status", pw->pw_dir, upgradeId);
                unlink(download_status_file);
            } else if (strcmp(status, "downloaded") == 0) {
                // 记录下载状态
                char download_status_file[1024];
                snprintf(download_status_file, sizeof(download_status_file), 
                    "%s/.%s_download_status", pw->pw_dir, upgradeId);
                FILE *df = fopen(download_status_file, "w");
                if (df) {
                    fprintf(df, "downloaded\n");
                    fclose(df);
                }
            }
        } else {
            printf("日志上传失败: %s\n", curl_easy_strerror(res));
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return success;
}
// 上传下载进度，先获取upgradeId,支持下载成功和失败两种情况
int upload_download_progress_with_upgradeid(const char *filename, int success, const char *token) {
    char upgradeId[128] = {0};
    if (get_upgrade_id_from_backend(upgradeId, sizeof(upgradeId), filename, token)==0) {
        printf("获取upgradeId失败，无法上报进度！\n");
        return 0;
    }
    char json_body[512];
    if (success) {
        printf("固件下载成功，开始上报进度...\n");
        snprintf(json_body, sizeof(json_body),
            "{"
                "\"upgradeId\":\"%s\","
                "\"progress\":100,"
                "\"status\":\"downloaded\","
                "\"message\":\"固件下载完成\""
            "}", upgradeId);
    } else {
        printf("固件下载失败，开始上报进度...\n");
        snprintf(json_body, sizeof(json_body),
            "{"
                "\"upgradeId\":\"%s\","
                "\"progress\":0,"
                "\"status\":\"downloaded\","
                "\"message\":\"固件下载失败\""
            "}", upgradeId);
    }
    // 用curl命令行，带Authorization
    char curl_cmd[2048];
    snprintf(curl_cmd, sizeof(curl_cmd),
        "curl -s -X POST \"https://seeky.cc/api/ota/upgrade-progress/all\" "
        "-H \"Authorization: Bearer %s\" "
        "-H \"Content-Type: application/json\" "
        "-d '%s'", token, json_body);

    printf("上传下载进度到后台...(success=%d, upgradeId=%s)\n", success, upgradeId);
    int ret = system(curl_cmd);
    if (ret == 0) {
        printf("进度上传成功\n");
        
        // 记录下载状态到本地文件
        if (success) {
            struct passwd *pw = getpwuid(getuid());
            char download_status_file[1024];
            snprintf(download_status_file, sizeof(download_status_file), 
                "%s/.%s_download_status", pw->pw_dir, upgradeId);
            FILE *df = fopen(download_status_file, "w");
            if (df) {
                fprintf(df, "downloaded\n");
                fclose(df);
            }
        }
        return 1;
    } else {
        printf("进度上传失败\n");
        return 0;
    }
}
/**
 * @brief 获取API最新固件信息并下载，解析bin id、文件名、目标设备id
 * @param bin_id 输出的固件ID
 * @param id_len bin_id缓冲区大小
 * @param bin_filename 输出的固件文件名
 * @param filename_len filename缓冲区大小
 * @param download_path 输出的下载路径
 * @param path_len download_path缓冲区大小
 * @param target_device_id 输出的目标设备ID
 * @param dev_id_len target_device_id缓冲区大小
 * @param token API令牌
 * @param is_single_device 是否为单设备升级模式
 * @return 1成功，0失败
 */
int get_latest_bin_from_api(char *bin_id, size_t id_len,char *bin_filename, size_t filename_len,char *download_path, size_t path_len,char *target_device_id, size_t dev_id_len,const char *token, int is_single_device)
{
    char json_file[512];
    char curl_download_cmd[1024];
    
    // 生成临时JSON文件路径（存储latest接口返回的固件基础信息）
    snprintf(json_file, sizeof(json_file), "/tmp/fw_latest_%d.json", getpid());
    
    // 第一步：调用latest接口获取固件基础信息（id、filename、deviceId）
    snprintf(curl_download_cmd, sizeof(curl_download_cmd), 
        "curl -X GET \"https://seeky.cc/api/firmware/latest?deviceType=robot\" -o \"%s\"", json_file);
    if (system(curl_download_cmd) != 0) {
        printf("curl获取latest接口失败！\n");
        return 0;
    }
    FILE *fp = fopen(json_file, "r");
    if (!fp) {
        printf("无法打开JSON临时文件！\n");
        unlink(json_file);
        return 0;
    }
    fseek(fp, 0, SEEK_END);
    long json_size = ftell(fp);
    rewind(fp);
    char *json_buf = malloc(json_size+1);
    if (!json_buf) {
        fclose(fp); unlink(json_file); return 0;
    }
    fread(json_buf, 1, json_size, fp);
    json_buf[json_size] = 0;
    fclose(fp); 
    unlink(json_file); // 读取完成后删除临时文件
    
    // 解析latest接口返回的JSON，提取固件基础信息
    cJSON *root = cJSON_Parse(json_buf);
    if (!root) { printf("cJSON_Parse(latest接口)失败！\n"); free(json_buf); return 0; }
    cJSON *firmware = cJSON_GetObjectItem(root, "firmware");
    if (!firmware) { printf("未找到firmware字段！\n"); cJSON_Delete(root); free(json_buf); return 0; }
    cJSON *id_item = cJSON_GetObjectItem(firmware, "id");
    cJSON *filename_item = cJSON_GetObjectItem(firmware, "filename");
    cJSON *target_id_item = cJSON_GetObjectItem(firmware, "deviceId");
    if (!id_item || !filename_item || !target_id_item) {
        printf("id、filename或deviceId不存在！\n");
        cJSON_Delete(root); free(json_buf); return 0;
    }
    // 保存固件基础信息到输出参数
    strncpy(bin_id, id_item->valuestring, id_len-1); 
    bin_id[id_len-1] = 0;
    strncpy(bin_filename, filename_item->valuestring, filename_len-1); 
    bin_filename[filename_len-1] = 0;
    strncpy(target_device_id, target_id_item->valuestring, dev_id_len-1); 
    target_device_id[dev_id_len-1] = 0;
    cJSON_Delete(root); 
    free(json_buf);
    
    // 第二步：构建"获取下载链接"的接口URL
    char get_dl_url[512];
    snprintf(get_dl_url, sizeof(get_dl_url), "https://seeky.cc/api/firmware/%s/download-url", bin_id);
    printf("API获取最新bin: id=%s, filename=%s, 目标设备ID=%s\n",
           bin_id, bin_filename, target_device_id);
    printf("第一步：请求下载链接的接口URL: %s\n", get_dl_url);
    
    // 第三步：调用"获取下载链接"接口，获取包含downloadUrl的JSON响应
    char dl_url_json[512]; // 存储下载链接接口的响应JSON
    snprintf(dl_url_json, sizeof(dl_url_json), "/tmp/fw_dl_url_%d.json", getpid());
    char curl_get_dl_url_cmd[2048];
    snprintf(curl_get_dl_url_cmd, sizeof(curl_get_dl_url_cmd),
        "curl -X GET \"%s\" -o \"%s\"", get_dl_url, dl_url_json);
    if (system(curl_get_dl_url_cmd) != 0) {
        printf("curl请求下载链接接口失败！\n");
        unlink(dl_url_json);
        return 0;
    }
    
    // 第四步：解析下载链接接口的JSON响应，提取downloadUrl
    FILE *dl_fp = fopen(dl_url_json, "r");
    if (!dl_fp) {
        printf("无法打开下载链接接口的JSON文件！\n");
        unlink(dl_url_json);
        return 0;
    }
    fseek(dl_fp, 0, SEEK_END);
    long dl_json_size = ftell(dl_fp);
    rewind(dl_fp);
    char *dl_json_buf = malloc(dl_json_size + 1);
    if (!dl_json_buf) {
        fclose(dl_fp); unlink(dl_url_json); return 0;
    }
    fread(dl_json_buf, 1, dl_json_size, dl_fp);
    dl_json_buf[dl_json_size] = 0;
    fclose(dl_fp);
    unlink(dl_url_json); // 读取完成后删除临时文件
    
    // 解析JSON中的downloadUrl字段（核心修改：提取实际下载地址）
    cJSON *dl_root = cJSON_Parse(dl_json_buf);
    if (!dl_root) { 
        printf("cJSON_Parse(下载链接接口)失败！响应内容：%s\n", dl_json_buf);
        free(dl_json_buf); 
        return 0; 
    }
    cJSON *downloadUrl_item = cJSON_GetObjectItem(dl_root, "downloadUrl");
    if (!downloadUrl_item || !cJSON_IsString(downloadUrl_item) || strlen(downloadUrl_item->valuestring) == 0) {
        printf("未找到有效的downloadUrl字段！响应内容：%s\n", dl_json_buf);
        cJSON_Delete(dl_root);
        free(dl_json_buf);
        return 0;
    }
    char real_download_url[1024]; // 实际的固件下载地址
    strncpy(real_download_url, downloadUrl_item->valuestring, sizeof(real_download_url)-1);
    real_download_url[sizeof(real_download_url)-1] = 0;
    cJSON_Delete(dl_root);
    free(dl_json_buf);
    printf("第二步：解析到实际固件下载URL: %s\n", real_download_url);
	
	
    
    // 第五步：确定固件本地保存路径（用户主目录下）
    struct passwd *pw1 = getpwuid(getuid());
    if (!pw1) {
        printf("获取用户目录失败！\n");
        return 0;
    }
    const char *homedir = pw1->pw_dir;
    snprintf(download_path, path_len, "%s/%s", homedir, bin_filename);
    
    
    

    printf("API获取最新bin: id=%s, filename=%s, 目标设备ID=%s\n下载URL: %s\n保存路径: %s\n",
           bin_id, bin_filename, target_device_id, real_download_url, download_path);

    //下载bin文件到本地
    // 检查升级完成状态和下载状态的路径
    char ota_status_path[1024];
    char dl_status_path[1024];
    snprintf(ota_status_path, sizeof(ota_status_path), "%s/.ota_status_%s", homedir, bin_id);
    snprintf(dl_status_path, sizeof(dl_status_path), "%s/.%s_download_status", homedir, bin_id);
    
    // 检查是否已完成升级
    FILE *check_sf = fopen(ota_status_path, "r");
    if (check_sf) {
        char curr_status[64] = {0};
        if (fscanf(check_sf, "%63s", curr_status) == 1) {
            if (strcmp(curr_status, "completed") == 0) {
                printf("固件已经完成升级，跳过下载和状态上报。\n");
                fclose(check_sf);
                return 1;
            }
        }
        fclose(check_sf);
    }

    // 检查文件是否已存在且下载状态文件存在
    if (access(download_path, F_OK) == 0) {  // 文件存在
        FILE *check_df = fopen(dl_status_path, "r");
        if (check_df) {
            char dl_status[64] = {0};
            if (fscanf(check_df, "%63s", dl_status) == 1) {
                if (strcmp(dl_status, "downloaded") == 0) {
                    printf("固件文件已存在且下载完成，跳过下载和状态上报。\n");
                    fclose(check_df);
                    return 1;
                }
            }
            fclose(check_df);
        }
    }

    //下载bin文件到本地
    // 先检查是否已经下载过此固件并完成升级
    struct passwd *pw = getpwuid(getuid());
    char status_file[1024];
    snprintf(status_file, sizeof(status_file), "%s/.ota_status_%s", pw->pw_dir, bin_id);
    FILE *sf = fopen(status_file, "r");
    if (sf) {
        char status[64] = {0};
        if (fscanf(sf, "%63s", status) == 1) {
            if (strcmp(status, "completed") == 0) {
                printf("固件已经完成升级，无需重新下载和上报。\n");
                fclose(sf);
                return 1;
            }
        }
        fclose(sf);
    }
    
    printf("第四步：开始下载固件...\n");
	// 第七步：调用实际下载链接，下载固件到本地
    char curl_real_dl_cmd[2048];
    snprintf(curl_real_dl_cmd, sizeof(curl_real_dl_cmd), 
        "curl -s -L \"%s\" -o \"%s\"", real_download_url, download_path); // -L支持重定向（应对OSS等存储的跳转）
    int download_success = 1;
    if (system(curl_real_dl_cmd) != 0) {
        printf("curl下载固件失败！命令：%s\n", curl_real_dl_cmd);
        download_success = 0;
    }
    // 二次校验文件是否存在（避免curl返回0但实际下载失败的情况）
    if (access(download_path, F_OK) != 0) {
        printf("固件文件未下载到本地！路径：%s\n", download_path);
        download_success = 0;
    }
    
    // 第八步：处理下载结果（记录状态、绑定upgradeId）
    char *filename = strrchr(bin_filename, '/');
    filename = filename ? filename + 1 : (char*)bin_filename;
    
    // 记录下载状态到本地文件（用于后续检查）
    char download_status_file[1024];
    snprintf(download_status_file, sizeof(download_status_file), 
        "%s/.%s_download_status", pw->pw_dir, bin_id);
    
    if (download_success) {
        FILE *df = fopen(download_status_file, "w");
        if (df) {
            fprintf(df, "downloaded\n");
            fclose(df);
        }
        // 为单设备升级保存upgradeId映射（但不上报日志）
        if (is_single_device) {
            set_upgrade_id_by_filename(filename, bin_id);
        }
        printf("固件下载成功！\n");
    } else {
        // 下载失败时删除残留文件
        if (access(download_path, F_OK) == 0) {
            unlink(download_path);
        }
        printf("固件下载失败！\n");
    }
    
    return download_success;

}

/**
 * @brief 获取上次下载升级过的固件ID
 * @return 1有记录，0无记录
 */
int get_last_downloaded_bin_id(char *id, size_t len) {
    struct passwd *pw = getpwuid(getuid());
    char path[1024];
    snprintf(path, sizeof(path), "%s/latest_bin_id", pw->pw_dir);
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    if (fgets(id, len, fp) == NULL) {
        fclose(fp);
        return 0;
    }
    id[strcspn(id, "\r\n")] = 0; // 去掉换行
    fclose(fp);
    return 1;
}
/**
 * @brief 保存已升级固件ID到本地
 */
void set_last_downloaded_bin_id(const char *id) {
    struct passwd *pw = getpwuid(getuid());
    char path[1024];
    snprintf(path, sizeof(path), "%s/latest_bin_id", pw->pw_dir);
    FILE *fp = fopen(path, "w");
    if (!fp) return;
    fprintf(fp, "%s\n", id);
    fclose(fp);
}

/**
 * @brief 获取设备的升级任务信息（通过统一的latest接口）
 * @param device_id 设备ID
 * @param bin_id 输出的固件ID
 * @param id_len bin_id缓冲区大小
 * @param filename 输出的固件文件名
 * @param filename_len filename缓冲区大小
 * @param download_path 输出的下载路径
 * @param path_len download_path缓冲区大小
 * @param target_device_id 输出的目标设备ID
 * @param dev_id_len target_device_id缓冲区大小
 * @param token API令牌
 * @return 1有升级任务，0无升级任务（正常），-1错误
 */
int get_device_upgrade_info(const char *device_id, char *bin_id, size_t id_len,
                           char *filename, size_t filename_len,
                           char *download_path, size_t path_len,
                           char *target_device_id, size_t dev_id_len, const char *token) {
    // 调用统一的latest接口获取最新固件信息
    // 第一次调用只是获取信息，不进行实际的状态上报
    if (!get_latest_bin_from_api(bin_id, id_len, filename, filename_len,
                                download_path, path_len, target_device_id, dev_id_len, token, 0)) {
        printf("获取最新固件信息失败\n");
        return -1;
    }

    // 检查目标设备ID是否匹配当前设备或者是全部设备升级
    if (strcmp(target_device_id, device_id) == 0 || strcmp(target_device_id, "all") == 0) {
        printf("检测到适用的升级任务:\n");
        printf("  固件ID: %s\n", bin_id);
        printf("  文件名: %s\n", filename);
        printf("  目标设备: %s\n", target_device_id);
        printf("  下载路径: %s\n", download_path);
        return 1;  // 有适用的升级任务
    } else {
        printf("当前固件目标设备为 %s，与本设备 %s 不匹配，无升级任务\n", target_device_id, device_id);
        return 0;  // 无适用的升级任务
    }
}

/**
 * @brief 获取单台设备的升级任务ID
 * @param device_id 设备ID
 * @param task_id 输出的任务ID
 * @param task_id_len 任务ID缓冲区大小
 * @param token API令牌
 * @return 1成功，0失败
 */
int get_single_device_task_id(const char *device_id, char *task_id, size_t task_id_len, const char *token) {
    CURL *curl;
    CURLcode res;
    struct mem_chunk response = {0};
    response.memory = malloc(1);
    response.size = 0;
    
    int success = 0;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl) {
        // 构建获取单设备任务ID的接口URL
        char url[512];
        snprintf(url, sizeof(url), "https://seeky.cc/api/ota/upgrade-info/%s", device_id);
        
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

        struct curl_slist *headers = NULL;
        char auth_header[512];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", token);
        headers = curl_slist_append(headers, auth_header);
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        printf("获取单设备任务ID，URL: %s\n", url);
        
        res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            printf("获取任务ID成功，服务器响应: %s\n", response.memory);
            
            // 解析JSON响应获取任务ID
            cJSON *root = cJSON_Parse(response.memory);
            if (root) {
                cJSON *task_id_item = cJSON_GetObjectItem(root, "upgradeId");
                if (task_id_item && cJSON_IsString(task_id_item)) {
                    strncpy(task_id, task_id_item->valuestring, task_id_len - 1);
                    task_id[task_id_len - 1] = 0;
                    success = 1;
                    printf("获取到任务ID: %s\n", task_id);
                } else {
                    printf("响应中没有找到taskId字段\n");
                }
                cJSON_Delete(root);
            } else {
                printf("解析JSON响应失败\n");
            }
        } else {
            printf("获取任务ID失败: %s\n", curl_easy_strerror(res));
        }

        free(response.memory);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return success;
}

/**
 * @brief 上传单台设备的下载/升级日志
 * @param task_id 任务ID
 * @param progress 进度百分比
 * @param status 状态（downloaded/completed/failed）
 * @param message 消息内容
 * @param token API令牌
 * @return 1成功，0失败
 */
int upload_single_device_log(const char *task_id, int progress, const char *status, 
                           const char *message, const char *token) {
    CURL *curl;
    CURLcode res;
    struct mem_chunk response = {0};
    response.memory = malloc(1);
    response.size = 0;
    
    int success = 0;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl) {
        // 构建单设备升级进度接口URL，使用任务ID
        char url[512];
        snprintf(url, sizeof(url), "https://seeky.cc/api/ota/upgrade-progress/%s", task_id);
        
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

        struct curl_slist *headers = NULL;
        char auth_header[512];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", token);
        headers = curl_slist_append(headers, auth_header);
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // 准备POST数据
        char data[512];
        const char *json_fmt = "{\n"
            "  \"upgradeId\": \"%s\",\n"
            "  \"progress\": %d,\n"
            "  \"status\": \"%s\",\n"
            "  \"message\": \"%s\"\n"
            "}";
        snprintf(data, sizeof(data), json_fmt, task_id, progress, status, message);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        
        printf("发送数据: %s\n", data);
        
        res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            printf("单设备日志上传成功: %s - %s\n", status, message);
            printf("服务器响应: %s\n", response.memory);
            success = 1;
        } else {
            printf("单设备日志上传失败: %s\n", curl_easy_strerror(res));
        }

        free(response.memory);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return success;
}

/**
 * @brief 执行设备的完整OTA升级流程（通用函数，支持单台和全部设备）
 * @param device_id 设备ID
 * @param token API令牌
 * @return 1成功，0失败
 */
int do_device_ota_upgrade(const char *device_id, const char *token) {
    char bin_id[128] = {0};
    char filename[256] = {0};
    char download_path[1024] = {0};
    char target_device_id[128] = {0};

    printf("=== 开始设备OTA升级流程 ===\n");
    printf("设备ID: %s\n", device_id);

    // 1. 获取设备的升级任务信息（通过统一的latest接口）
    printf("\n步骤1: 获取升级任务信息...\n");
    int upgrade_info_result = get_device_upgrade_info(device_id, bin_id, sizeof(bin_id),
                                                     filename, sizeof(filename),
                                                     download_path, sizeof(download_path),
                                                     target_device_id, sizeof(target_device_id), token);
    
    if (upgrade_info_result == 0) {
        // 无升级任务，正常情况
        printf("当前没有待处理的升级任务，程序正常退出。\n");
        return 1;  // 返回1表示正常完成（虽然没有升级任务）
    } else if (upgrade_info_result == -1) {
        // API错误或数据格式错误
        printf("获取升级任务信息失败！\n");
        return 0;
    }
    // upgrade_info_result == 1，有升级任务，继续执行

    // 检查是否已完成升级
    struct passwd *pw = getpwuid(getuid());
    if (!pw) {
        printf("获取用户目录失败！\n");
        return 0;
    }

    char status_file[1024];
    snprintf(status_file, sizeof(status_file), 
             "%s/.ota_status_%s", pw->pw_dir, bin_id);

    FILE *sf = fopen(status_file, "r");
    if (sf) {
        char status[64] = {0};
        if (fscanf(sf, "%63s", status) == 1 && strcmp(status, "completed") == 0) {
            printf("固件 %s 已完成升级，跳过操作\n", filename);
            fclose(sf);
            return 1;
        }
        fclose(sf);
    }
    

    // 2. 固件已经通过get_latest_bin_from_api下载完成
    printf("\n步骤2: 固件下载检查...\n");
    printf("固件路径: %s\n", download_path);

    if (access(download_path, F_OK) != 0) {
        printf("固件文件不存在！\n");
        return 0;
    }
    printf("固件文件确认存在，继续升级流程\n");

    // 2.5. 检查并上报固件下载完成日志（避免重复上报）
    printf("\n步骤2.5: 检查下载完成状态...\n");
    int is_single_device = (strcmp(target_device_id, "all") != 0);
    printf("is_single_device: %d\n", is_single_device);
    char *short_filename = strrchr(filename, '/');
    short_filename = short_filename ? short_filename + 1 : filename;
    
    // 检查是否已经上报过下载完成状态
    char download_status_file[1024];
    snprintf(download_status_file, sizeof(download_status_file), 
        "%s/.%s_download_reported", pw->pw_dir, bin_id);
    
    //FILE *reported_check = fopen(download_status_file, "r");
    //if (!reported_check) {
        // 还没上报过下载完成状态，现在上报
        printf("上报下载完成状态到后台...\n");
        
        if (is_single_device) {
            // 单设备升级：先获取任务ID，然后上报下载完成状态
            printf("单设备升级，获取任务ID并上报下载完成状态\n");
            
            char task_id[128] = {0};
            if (get_single_device_task_id(local_device_id, task_id, sizeof(task_id), token)) {
                // 使用任务ID上报下载状态
                upload_single_device_log(
                    task_id,
                    100,
                    "downloaded", 
                    "固件下载完成",
                    token
                );
                
                // 保存任务ID映射（用于升级完成时使用）
                set_upgrade_id_by_filename(short_filename, task_id);
            } else {
                printf("获取单设备任务ID失败，无法上报下载完成状态\n");
            }
        } else {
            // 全部设备升级：使用原有API
            printf("全部设备升级，上报下载完成状态\n");
            upload_ota_log_to_backend(
                short_filename,
                100,
                "downloaded",
                "固件下载完成", 
                token
            );
        }
        
        // 标记已上报下载完成状态
        FILE *reported_flag = fopen(download_status_file, "w");
        if (reported_flag) {
            fprintf(reported_flag, "reported\n");
            fclose(reported_flag);
        }
    //} else {
        //fclose(reported_check);
        //printf("下载完成状态已上报过，跳过重复上报\n");
    //}

    // 3. 执行OTA升级
    printf("\n步骤3: 执行OTA升级...\n");
    int ota_result = do_ota_upgrade(download_path, (strcmp(target_device_id, "all") != 0), bin_id, token);
    int ota_success = (ota_result == 0);

    // 记录完成状态到本地
    if (ota_success) {
        FILE *sf = fopen(status_file, "w");
        if (sf) {
            fprintf(sf, "completed\n");
            fclose(sf);
        }
        printf("\n=== 设备OTA升级成功！ ===\n");
    } else {
        printf("\n=== 设备OTA升级失败！ ===\n");
    }

    return ota_success;
}

/**
 * @brief OTA主流程：bin路径->CAN口升级
 * @param binfile 固件文件路径
 * @param is_single_device 是否为单台设备升级
 * @param bin_id 固件ID（用于单台设备升级的upgradeId）
 * @param token API令牌
 * @return 0成功，1失败
 */
int do_ota_upgrade(const char *binfile, int is_single_device, const char *bin_id, const char *token) {
    int sock = -1;
    struct sockaddr_can addr;
    struct ifreq ifr;
    pid_t candump_pid = -1;
    uint8_t *file_data = NULL;
    int ota_success = 0;

    // 1. 配置CAN接口、启动candump日志
    system("sudo ip link set down can0");
    system("sudo ip link set can0 type can bitrate 500000");
    system("sudo modprobe slcan");
    system("sudo ip link set up can0");
    candump_pid = fork();
    if (candump_pid == 0) {
        int fd = open("canlog.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644);
        if (fd >= 0) {
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }
        execlp("candump", "candump", "can0", NULL);
        perror("exec candump failed");
        exit(1);
    } else if (candump_pid > 0) {
        printf("candump started, pid=%d\n", candump_pid);
    } else {
        perror("fork failed");
        return 1;
    }

    // 2. 读取bin文件到内存
    FILE *fp = fopen(binfile, "rb");
    if (!fp) {
        perror("open bin");
        cleanup(&sock, file_data, candump_pid);
        return 1;
    }
    fseek(fp, 0, SEEK_END);
    int file_len = ftell(fp);

    rewind(fp);
    file_data = malloc(file_len);
    if (!file_data) {
        printf("内存分配失败！\n");
        fclose(fp);
        cleanup(&sock, file_data, candump_pid);
        return 1;
    }
    fread(file_data, 1, file_len, fp);
    fclose(fp);

    printf("OTA升级文件：%s, 大小：%d 字节\n", binfile, file_len);

    // 3. 打开SocketCAN can0
    if ((sock = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("socket");
        cleanup(&sock, file_data, candump_pid);
        return 1;
    }
    strcpy(ifr.ifr_name, "can0");
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl");
        cleanup(&sock, file_data, candump_pid);
        return 1;
    }
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        cleanup(&sock, file_data, candump_pid);
        return 1;
    }

    // 4. 发送OTA请求帧
    int sent = 0;
    uint8_t frame_cnt = 1;
    int first_data = 0; // 第一帧发0字节数据
    int remain = file_len - first_data;
    uint8_t ota_req[8] = {0};
    ota_req[0] = remain;
    ota_req[1] = (file_len >> 24) & 0xFF;
    ota_req[2] = (file_len >> 16) & 0xFF;
    ota_req[3] = (file_len >> 8) & 0xFF;
    ota_req[4] = file_len & 0xFF;
    ota_req[5] = first_data > 0 ? file_data[0] : 0;
    ota_req[6] = frame_cnt;
    ota_req[7] = calc_xor(ota_req);
    int ret_val=send_frame_wait_ack(sock, OTA_CAN_REQUEST_ID, ota_req, frame_cnt);
    if (ret_val!=0) {
        printf("ret_val=%d,OTA请求失败\n",ret_val);
        cleanup(&sock, file_data, candump_pid);
        return 1;
    }
    printf("下位机ACK，开始分帧发送数据...\n");
    sent += first_data;
    frame_cnt = (frame_cnt + 1) % 256;

    // 5. 发送后续帧，每帧最多带4字节有效数据
    int fail = 0;
    while (sent < file_len) {
        remain = file_len - sent;
        int datalen = (remain >= 4) ? 4 : remain;
        uint8_t frame[8] = {0};
        frame[0] = remain - datalen;
        memcpy(&frame[1], &file_data[sent], datalen);
        frame[5] = 0x00;
        frame[6] = frame_cnt;
        frame[7] = calc_xor(frame);
        printf("frame[1-4]:%02X %02X %02X %02X \n",frame[1],frame[2],frame[3],frame[4]);
        if (send_frame_wait_ack(sock, OTA_CAN_DATA_ID, frame, frame_cnt)!=0) {
            printf("帧cnt=%u发送失败\n", frame_cnt);
            fail = 1;
            cleanup(&sock, file_data, candump_pid);
            return 1;
        }
        sent += datalen;
        frame_cnt = (frame_cnt + 1) % 256;
        print_progress(sent, file_len);
    }
    putchar('\n');
    if (!fail && sent >= file_len) {
        printf("OTA升级全部数据发送完成。\n");
        ota_success = 1;
        log_ota_upgrade(binfile, file_len, "SUCCESS", file_data);
    } else {
        log_ota_upgrade(binfile, file_len, "FAIL_SEND", file_data);
    }

    // 确保获取到正确的upgradeId
    char upgradeId[128] = {0};
    char *filename = strrchr(binfile, '/');
    filename = filename ? filename + 1 : (char*)binfile;
    
    if (is_single_device) {
        // 单台设备升级：使用保存的任务ID作为upgradeId
        if (get_upgrade_id_by_filename(filename, upgradeId, sizeof(upgradeId))) {
            printf("单台设备升级，使用保存的任务ID作为upgradeId: %s\n", upgradeId);
        } else {
            printf("单台设备升级，但未找到保存的任务ID，使用bin_id作为备用: %s\n", bin_id);
            strncpy(upgradeId, bin_id, sizeof(upgradeId) - 1);
            upgradeId[sizeof(upgradeId) - 1] = 0;
        }
        
        // 保存到本地映射
        set_upgrade_id_by_filename(filename, upgradeId);
    } else {
        // 全部设备升级：使用原有逻辑
        if (!get_upgrade_id_by_filename(filename, upgradeId, sizeof(upgradeId))) {
            printf("从本地获取upgradeId失败，尝试从API获取...\n");
            if (!get_upgrade_id_from_backend(upgradeId, sizeof(upgradeId), filename, token)) {
                printf("获取upgradeId完全失败，无法上报升级完成日志！\n");
                // 即使失败也继续执行清理操作
                cleanup(&sock, file_data, candump_pid);
                return ota_success ? 0 : 1;
            }
        }
    }
    
    // 检查是否已经完成升级
    struct passwd *pw = getpwuid(getuid());
    char status_file[1024];
    snprintf(status_file, sizeof(status_file), "%s/.ota_status_%s", pw->pw_dir, upgradeId);
    FILE *sf = fopen(status_file, "r");
    if (sf) {
        char status[64] = {0};
        if (fscanf(sf, "%63s", status) == 1 && strcmp(status, "completed") == 0) {
            printf("固件已经完成升级，跳过重复升级。\n");
            fclose(sf);
            cleanup(&sock, file_data, candump_pid);
            return 0;
        }
        fclose(sf);
    }

    printf("升级完成，使用upgradeId=%s上报日志\n", upgradeId);
    
    // 等待2秒，确保与下载完成状态上报有足够间隔
    sleep(2);

    // 直接使用CURL调用API，根据升级类型选择不同的端点
    CURL *curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl) {
        struct curl_slist *headers = NULL;
        char auth_header[256];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", token);
        headers = curl_slist_append(headers, auth_header);
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        char post_data[512];
        char api_url[512];
        
        if (is_single_device) {
            // 单台设备升级使用特定的API端点
            snprintf(api_url, sizeof(api_url), "https://seeky.cc/api/ota/upgrade-progress/%s", upgradeId);
            snprintf(post_data, sizeof(post_data),
                "{\"upgradeId\":\"%s\",\"progress\":100,\"status\":\"completed\",\"message\":\"固件升级完成\"}",
                upgradeId);
        } else {
            // 全部设备升级使用原有的API端点
            snprintf(api_url, sizeof(api_url), "https://seeky.cc/api/ota/upgrade-progress/all");
            snprintf(post_data, sizeof(post_data),
                "{\"upgradeId\":\"%s\",\"progress\":100,\"status\":\"completed\",\"message\":\"固件升级完成\"}",
                upgradeId);
        }
        
        curl_easy_setopt(curl, CURLOPT_URL, api_url);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");  // 明确指定 POST 方法
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
        
        printf("准备上报升级完成状态到 %s，post_data: %s\n", api_url, post_data);
        
        struct mem_chunk response;
        response.memory = malloc(1);
        response.size = 0;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
        
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            printf("升级日志上传失败: %s\n", curl_easy_strerror(res));
        } else {
            printf("升级日志上传成功，服务器响应: %s\n", response.memory);
            
            // 如果是completed状态，记录到本地文件并清除下载状态
            if (strstr(post_data, "\"status\":\"completed\"")) {
                struct passwd *pw = getpwuid(getuid());
                // 记录完成状态
                char status_file[1024];
                snprintf(status_file, sizeof(status_file), "%s/.ota_status_%s", pw->pw_dir, upgradeId);
                FILE *sf = fopen(status_file, "w");
                if (sf) {
                    fprintf(sf, "completed\n");
                    fclose(sf);
                }
                
                // 清除下载状态相关文件
                char download_status_file[1024];
                snprintf(download_status_file, sizeof(download_status_file), 
                    "%s/.%s_download_status", pw->pw_dir, upgradeId);
                unlink(download_status_file);  // 删除下载状态文件
                
                // 清除下载完成上报标记文件
                char download_reported_file[1024];
                snprintf(download_reported_file, sizeof(download_reported_file), 
                    "%s/.%s_download_reported", pw->pw_dir, upgradeId);
                unlink(download_reported_file);  // 删除下载完成上报标记文件
            }
        }
        
        if(response.memory) free(response.memory);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    cleanup(&sock, file_data, candump_pid);
    return ota_success ? 0 : 1;
}

/**
 * @brief 主流程: timerfd定时检测API, 校验目标设备id, 自动OTA升级（统一处理单台和全部设备）
 */
int main(int argc, char *argv[]) {
    // 1. 获取本地设备ID
    if (!get_local_device_id(local_device_id, sizeof(local_device_id))) {
        fprintf(stderr, "本地设备ID获取失败，无法OTA自动升级！\n");
        return 1;
    }
    printf("本机设备ID: %s\n", local_device_id);

    // 2. 创建定时器，每CHECK_INTERVAL_SEC秒检测一次
    int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timer_fd == -1) {
        perror("timerfd_create");
        exit(1);
    }
    struct itimerspec its;
    its.it_interval.tv_sec = CHECK_INTERVAL_SEC;
    its.it_interval.tv_nsec = 0;
    its.it_value.tv_sec = 1; // 启动后1s首次检测
    its.it_value.tv_nsec = 0;
    if (timerfd_settime(timer_fd, 0, &its, NULL) == -1) {
        perror("timerfd_settime");
        exit(1);
    }

    printf("启动OTA定时检测，周期%d秒。\n", CHECK_INTERVAL_SEC);

    struct pollfd fds[1];
    fds[0].fd = timer_fd;
    fds[0].events = POLLIN;
    
    while (1) {
        int ret = poll(fds, 1, -1);
        if (ret < 0) {
            perror("poll");
            break;
        }
        if (fds[0].revents & POLLIN) {
            uint64_t exp;
            read(timer_fd, &exp, sizeof(exp));  // 必须读出，防止下一次不触发

            // ==== 统一的OTA检查逻辑 ====
            printf("\nOTA定时检测: 开始检查升级任务...\n");
            
            // 3. 调用通用的设备OTA升级函数
            int upgrade_result = do_device_ota_upgrade(local_device_id, api_token);
            
            if (upgrade_result == 1) {
                printf("OTA升级流程完成（可能是成功升级或无升级任务）。\n");
            } else {
                printf("OTA升级流程失败。\n");
            }
        }
    }
    close(timer_fd);
    return 0;
}
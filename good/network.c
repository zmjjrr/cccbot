#define _CRT_SECURE_NO_WARNINGS
#include "header.h"

SOCKET TCPhandler(const char* server, const char* baseNick)
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("WSAStartup failed\n");
        return INVALID_SOCKET;
    }

    SOCKET tcpsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tcpsock == INVALID_SOCKET)
    {
        printf("Socket creation failed\n");
        WSACleanup();
        return INVALID_SOCKET;
    }

    // 获取服务器地址信息
    struct addrinfo hints = { 0 }, * res = NULL;
    hints.ai_family = AF_UNSPEC;       // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    char port[6];
    sprintf(port, "%d", 6667);  // IRC 端口

    int result = getaddrinfo(server, port, &hints, &res);
    if (result != 0)
    {
        printf("getaddrinfo failed: %d\n", result);
        closesocket(tcpsock);
        WSACleanup();
        return INVALID_SOCKET;
    }

    // 尝试连接第一个可用地址
    if (connect(tcpsock, res->ai_addr, (int)res->ai_addrlen) == SOCKET_ERROR)
    {
        printf("Connection to IRC server failed\n");
        freeaddrinfo(res);
        closesocket(tcpsock);
        WSACleanup();
        return INVALID_SOCKET;
    }

    freeaddrinfo(res);

    printf("Connected to IRC server: %s\n", server);

    // 动态生成昵称
    char nick[128];
    int nickNum = 1;
    int connected = 0;

    while (!connected && nickNum <= 999) {
        snprintf(nick, sizeof(nick), "%s%03d", baseNick, nickNum);

        // 发送 NICK 命令
        char nickmsg[128];
        snprintf(nickmsg, sizeof(nickmsg), "NICK %s\r\n", nick);
        send(tcpsock, nickmsg, (int)strlen(nickmsg), 0);

        // 发送 USER 命令
        char usermsg[128];
        snprintf(usermsg, sizeof(usermsg), "USER %s 0 * :My Bot\r\n", nick);
        send(tcpsock, usermsg, (int)strlen(usermsg), 0);

        char buf[4096];
        int tried = 0;

        while (tried < 10) {
            int n = recv(tcpsock, buf, sizeof(buf) - 1, 0);
            if (n <= 0) {
                printf("Connection closed during handshake.\n");
                closesocket(tcpsock);
                WSACleanup();
                return INVALID_SOCKET;
            }

            buf[n] = '\0';

            // 处理 PING/PONG
            if (strncmp(buf, "PING", 4) == 0) {
                char pong[512];
                snprintf(pong, sizeof(pong), "PONG%s\r\n", buf + 4);
                send(tcpsock, pong, (int)strlen(pong), 0);
                continue;
            }

            // 检查是否昵称被占用
            if (strstr(buf, "433")) { // ERR_NICKNAMEINUSE
                printf("[!] Nickname '%s' is already in use. Trying next...\n", nick);
                break; // 退出当前循环，尝试下一个昵称
            }

            // 成功注册（RPL_WELCOME）
            if (strstr(buf, "001")) {
                printf("[+] Connected with nickname: %s\n", nick);
                connected = 1;
                break;
            }

            tried++;
        }

        if (connected) {
            break;
        }
        else {
            nickNum++;
            Sleep(1000); // 避免发送太快
        }
    }

    if (!connected) {
        printf("[!] Failed to register nickname after multiple attempts\n");
        closesocket(tcpsock);
        WSACleanup();
        return INVALID_SOCKET;
    }



    return tcpsock;
}
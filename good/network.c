#define _CRT_SECURE_NO_WARNINGS
#include "header.h"

SOCKET TCPhandler(const char* server, const char* baseNick)
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        bot_log("WSAStartup failed\n");
        return INVALID_SOCKET;
    }

    SOCKET tcpsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tcpsock == INVALID_SOCKET)
    {
        bot_log("Socket creation failed\n");
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
        bot_log("getaddrinfo failed: %d\n", result);
        closesocket(tcpsock);
        WSACleanup();
        return INVALID_SOCKET;
    }

    // 尝试连接第一个可用地址
    if (connect(tcpsock, res->ai_addr, (int)res->ai_addrlen) == SOCKET_ERROR)
    {
        bot_log("Connection to IRC server failed\n");
        freeaddrinfo(res);
        closesocket(tcpsock);
        WSACleanup();
        return INVALID_SOCKET;
    }

    freeaddrinfo(res);

    bot_log("Connected to IRC server: %s\n", server);

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
                bot_log("Connection closed during handshake.\n");
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
                bot_log("[!] Nickname '%s' is already in use. Trying next...\n", nick);
                break; // 退出当前循环，尝试下一个昵称
            }

            // 成功注册（RPL_WELCOME）
            if (strstr(buf, "001")) {
                bot_log("[+] Connected with nickname: %s\n", nick);
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
        bot_log("[!] Failed to register nickname after multiple attempts\n");
        closesocket(tcpsock);
        WSACleanup();
        return INVALID_SOCKET;
    }



    return tcpsock;
}

int check_join_success(SOCKET ircsock, const char* channel, DWORD timeout_ms) {
    char buffer[1024];
    DWORD startTime = GetTickCount();

    while (GetTickCount() - startTime < timeout_ms) {
        int received = recv(ircsock, buffer, sizeof(buffer) - 1, 0);
        if (received <= 0) {
            bot_log("Connection closed during JOIN check.\n");
            return -1; // Connection lost
        }

        buffer[received] = '\0'; // Null-terminate
        bot_log("JOIN Response: %s", buffer); // Optional log

        // Look for successful JOIN
        if (strstr(buffer, "JOIN") && strstr(buffer, channel)) {
            bot_log("Successfully joined channel: %s\n", channel);
            return 1; // Success
        }
        
        // send heartbeat
        if (strncmp(buffer, "PING", 4) == 0) {
            char pong[512];
            snprintf(pong, sizeof(pong), "PONG%s\r\n", buffer + 4);
            send(ircsock, pong, (int)strlen(pong), 0);
            continue;
        }

        // Error codes
        if (strstr(buffer, "403")) {
            bot_log("Error: No such channel or cannot join %s\n", channel);
            return 0;
        }
        if (strstr(buffer, "475")) {
            bot_log("Error: Bad channel key for %s\n", channel);
            return 0;
        }
        if (strstr(buffer, "473")) {
            bot_log("Error: Channel %s is invite-only\n", channel);
            return 0;
        }
        if (strstr(buffer, "471")) {
            bot_log("Error: Cannot join %s, channel is full\n", channel);
            return 0;
        }
        if (strstr(buffer, "474")) {
            bot_log("Error: You are banned from %s\n", channel);
            return 0;
        }

        // Reset buffer
        memset(buffer, 0, sizeof(buffer));
    }

    bot_log("Timeout waiting for JOIN confirmation.\n");
    return -1; // Timeout
}

int join_channel(const char* channel) {
    char joinCmd[128];
    snprintf(joinCmd, sizeof(joinCmd), "JOIN %s\r\n", channel);
    send(ircsock, joinCmd, (int)strlen(joinCmd), 0);

    int joinResult = check_join_success(ircsock, channel, 5000); // Wait up to 5 seconds
    if (joinResult != 1) {
        bot_log("Failed to join channel: %s\n", channel);
        closesocket(ircsock);
        return -1;
    }

    char privmsg[512];
    snprintf(privmsg, sizeof(privmsg), "PRIVMSG %s :Bot is online\r\n", channel);
    send(ircsock, privmsg, (int)strlen(privmsg), 0);
    return 0;
}

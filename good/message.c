#include "header.h"



// ��ʼ����Ϣ������
void initMessageBuffer(MessageBuffer* mb, const char* target) {
    memset(mb->buffer, 0, DEFAULT_BUFLEN);
    mb->length = 0;
    strncpy(mb->target, target, sizeof(mb->target) - 1);
    mb->lastSendTime = time(NULL);
}

// �����Ϣ������������Ҫʱ����
void appendToBuffer(MessageBuffer* mb, SOCKET sock, const char* message) {
    // ��������Ϣ���ȣ������ָ�����
    size_t msgLen = strlen(message);
    size_t newLen = mb->length + msgLen + 1; // +1 Ϊ�ָ������з�

    // ����������Ϣ�ᳬ����󳤶ȣ����߾����ϴη��ͳ���һ��ʱ�䣬���ȷ��͵�ǰ������
    if (newLen >= MAX_SEND_LENGTH || (time(NULL) - mb->lastSendTime) > 5) {
        if (mb->length > 0) { // ȷ�������ݿɷ���
            // �滻�ڲ����з�Ϊ�ո�IRC��Ϣ�л��з���ʾ��Ϣ������
            char* escapedBuffer = strdup(mb->buffer);

            for (char* p = escapedBuffer; *p; p++) {
                if (*p == '\n') *p = ' '; // ���ڲ������滻Ϊ�ո�
            }

            char sendBuf[DEFAULT_BUFLEN];

            snprintf(sendBuf, sizeof(sendBuf), "PRIVMSG %s :%s\r\n", mb->target, escapedBuffer);

            send(sock, sendBuf, (int)strlen(sendBuf), 0);

            free(escapedBuffer);

            // ��ջ�����
            memset(mb->buffer, 0, DEFAULT_BUFLEN);
            mb->length = 0;
            mb->lastSendTime = time(NULL);

            // ���Ʒ���Ƶ��
            Sleep(SEND_INTERVAL);
        }
    }

    // ������Ϣ��ӵ���������ʹ�û��з���Ϊ�ָ���
    if (mb->length > 0) {
        // ����������������ݣ���ӷָ������з�
        strncat(mb->buffer, "\n", DEFAULT_BUFLEN - mb->length - 1);
        mb->length++;
    }
    strncat(mb->buffer, message, DEFAULT_BUFLEN - mb->length - 1);
    mb->length += msgLen;
}

// ���ͻ�������ʣ���������Ϣ
void flushBuffer(MessageBuffer* mb, SOCKET sock) {
    if (mb->length > 0) {
        // �滻�ڲ����з�Ϊ�ո�
        char* escapedBuffer = strdup(mb->buffer);
        for (char* p = escapedBuffer; *p; p++) {
            if (*p == '\n') *p = ' '; // ���ڲ������滻Ϊ�ո�
        }

        char sendBuf[DEFAULT_BUFLEN];

        snprintf(sendBuf, sizeof(sendBuf), "PRIVMSG %s :%s\r\n", mb->target, escapedBuffer);

        send(sock, sendBuf, (int)strlen(sendBuf), 0);

        free(escapedBuffer);

        // ��ջ�����
        memset(mb->buffer, 0, DEFAULT_BUFLEN);
        mb->length = 0;
        mb->lastSendTime = time(NULL);

        // ���Ʒ���Ƶ��
        Sleep(SEND_INTERVAL);
    }
}
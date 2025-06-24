#include "header.h"



// 初始化消息缓冲区
void initMessageBuffer(MessageBuffer* mb, const char* target) {
    memset(mb->buffer, 0, DEFAULT_BUFLEN);
    mb->length = 0;
    strncpy(mb->target, target, sizeof(mb->target) - 1);
    mb->lastSendTime = time(NULL);
}

// 添加消息到缓冲区，必要时发送
void appendToBuffer(MessageBuffer* mb, SOCKET sock, const char* message) {
    // 计算新消息长度（包括分隔符）
    size_t msgLen = strlen(message);
    size_t newLen = mb->length + msgLen + 1; // +1 为分隔符换行符

    // 如果添加新消息会超过最大长度，或者距离上次发送超过一定时间，则先发送当前缓冲区
    if (newLen >= MAX_SEND_LENGTH || (time(NULL) - mb->lastSendTime) > 5) {
        if (mb->length > 0) { // 确保有内容可发送
            // 替换内部换行符为空格（IRC消息中换行符表示消息结束）
            char* escapedBuffer = strdup(mb->buffer);

            for (char* p = escapedBuffer; *p; p++) {
                if (*p == '\n') *p = ' '; // 将内部换行替换为空格
            }

            char sendBuf[DEFAULT_BUFLEN];

            snprintf(sendBuf, sizeof(sendBuf), "PRIVMSG %s :%s\r\n", mb->target, escapedBuffer);

            send(sock, sendBuf, (int)strlen(sendBuf), 0);

            free(escapedBuffer);

            // 清空缓冲区
            memset(mb->buffer, 0, DEFAULT_BUFLEN);
            mb->length = 0;
            mb->lastSendTime = time(NULL);

            // 控制发送频率
            Sleep(SEND_INTERVAL);
        }
    }

    // 将新消息添加到缓冲区，使用换行符作为分隔符
    if (mb->length > 0) {
        // 如果缓冲区已有内容，添加分隔符换行符
        strncat(mb->buffer, "\n", DEFAULT_BUFLEN - mb->length - 1);
        mb->length++;
    }
    strncat(mb->buffer, message, DEFAULT_BUFLEN - mb->length - 1);
    mb->length += msgLen;
}

// 发送缓冲区中剩余的所有消息
void flushBuffer(MessageBuffer* mb, SOCKET sock) {
    if (mb->length > 0) {
        // 替换内部换行符为空格
        char* escapedBuffer = strdup(mb->buffer);
        for (char* p = escapedBuffer; *p; p++) {
            if (*p == '\n') *p = ' '; // 将内部换行替换为空格
        }

        char sendBuf[DEFAULT_BUFLEN];

        snprintf(sendBuf, sizeof(sendBuf), "PRIVMSG %s :%s\r\n", mb->target, escapedBuffer);

        send(sock, sendBuf, (int)strlen(sendBuf), 0);

        free(escapedBuffer);

        // 清空缓冲区
        memset(mb->buffer, 0, DEFAULT_BUFLEN);
        mb->length = 0;
        mb->lastSendTime = time(NULL);

        // 控制发送频率
        Sleep(SEND_INTERVAL);
    }
}
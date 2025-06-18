#include "header.h"

// Unicode to ANSI conversion function
int UnicodeToAnsi(const wchar_t* unicodeStr, char* ansiStr, int bufferSize) {
    if (!unicodeStr || !ansiStr || bufferSize <= 0) {
        return -1; // Invalid input
    }

    int result = WideCharToMultiByte(CP_ACP, 0, unicodeStr, -1, ansiStr, bufferSize, NULL, NULL);
    if (result == 0) {
        return -2; // Conversion failed
    }

    return result - 1; // Exclude the null terminator from the count
}

// ANSI to Unicode conversion function
int AnsiToUnicode(const char* ansiStr, wchar_t* unicodeStr, int bufferSize) {
    if (!ansiStr || !unicodeStr || bufferSize <= 0) {
        return -1; // Invalid input
    }

    int result = MultiByteToWideChar(CP_ACP, 0, ansiStr, -1, unicodeStr, bufferSize);
    if (result == 0) {
        return -2; // Conversion failed
    }

    return result - 1; // Exclude the null terminator from the count
}

// UTF-8תANSI������ʹ������UnicodeToAnsi������
int Utf8ToAnsi(const char* utf8Str, char* ansiStr, int bufferSize) {
    if (!utf8Str || !ansiStr || bufferSize <= 0) {
        return -1; // ��������
    }

    // ��һ������UTF-8תΪUnicode�����ַ���
    int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, NULL, 0);
    if (wideLen == 0) return -2; // ��ȡ����ʧ��

    wchar_t* wideStr = (wchar_t*)malloc(wideLen * sizeof(wchar_t));
    if (!wideStr) return -3; // �ڴ����ʧ��

    if (MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, wideStr, wideLen) == 0) {
        free(wideStr);
        return -4; // ת��ʧ��
    }

    // �ڶ�������UnicodeתΪANSI��ʹ�����е�UnicodeToAnsi������
    int result = UnicodeToAnsi(wideStr, ansiStr, bufferSize);
    free(wideStr);
    return result;
}

int AnsiToUtf8(const char* ansiStr, char* utf8Str, int bufferSize) {
    if (!ansiStr || !utf8Str || bufferSize <= 0) {
        return -1; // ��������
    }

    // ��һ������ANSIתΪUnicode��ʹ�����е�AnsiToUnicode������
    int wideLen = AnsiToUnicode(ansiStr, NULL, 0) + 1; // +1 ������ֹ��
    if (wideLen <= 0) return -2; // ��ȡ����ʧ��

    wchar_t* wideStr = (wchar_t*)malloc(wideLen * sizeof(wchar_t));
    if (!wideStr) return -3; // �ڴ����ʧ��

    int res = AnsiToUnicode(ansiStr, wideStr, wideLen);
    if (res < 0) {
        free(wideStr);
        return -4; // ת��ʧ��
    }

    // �ڶ�������UnicodeתΪUTF-8
    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, NULL, 0, NULL, NULL);
    if (utf8Len == 0) {
        free(wideStr);
        return -5; // ��ȡ����ʧ��
    }

    if (utf8Len > bufferSize) {
        free(wideStr);
        return -6; // ����������
    }

    WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, utf8Str, utf8Len, NULL, NULL);
    free(wideStr);
    return utf8Len - 1; // �ų���ֹ��
}

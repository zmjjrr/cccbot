#include "header.h"

// ʹ�� PowerShell ѹ���ļ���Ϊ ZIP
int compress_with_powershell(const char* source_dir, const char* output_zip) {
    char command[1024];
    snprintf(command, sizeof(command),
        "powershell.exe -Command \"Compress-Archive -Path '%s\\*' -DestinationPath '%s' -Force\"",
        source_dir, output_zip);

    printf("[+] Compressing: %s -> %s\n", source_dir, output_zip);
    return run_executable(command, SW_HIDE);
}

// ʹ�� PowerShell ��ѹ ZIP �ļ�
int decompress_with_powershell(const char* zip_file, const char* dest_dir) {
    char command[1024];
    snprintf(command, sizeof(command),
        "powershell.exe -Command \"Expand-Archive -Path '%s' -DestinationPath '%s' -Force\"",
        zip_file, dest_dir);

    printf("[+] Decompressing: %s -> %s\n", zip_file, dest_dir);
    return run_executable(command, SW_HIDE);
}
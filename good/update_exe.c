#include"header.h"


int update_exe(char* path) {
    // ��ȡ��ǰ���� PID
    DWORD current_pid = GetCurrentProcessId();

    // ������ʱ bat �ļ�·��
    char temp_path[MAX_PATH];
    GetTempPathA(MAX_PATH, temp_path);
    char bat_path[MAX_PATH];
    sprintf_s(bat_path, MAX_PATH, "%update_launcher.bat", temp_path);

    FILE* fp = NULL;
    fopen_s(&fp, bat_path, "w");
    if (!fp) {
        bot_log("[-] Failed to create update script.\n");
        return -1;
    }

    // д������������,����cmd�������������
    fprintf(fp,
        "@echo off\n"
        "setlocal\n"
        "set PID=%lu\n"
        "echo Waiting for process %%PID%% to exit...\n"
        ":loop\n"
        "tasklist | findstr \"\\b%%PID%%\" >nul\n"
        "if %%errorlevel%% == 0 (\n"
        "    timeout /t 1 >nul\n"
        "    goto loop\n"
        ")\n"
        "echo Starting update: %s\n"
        "start \"\" \"%s\"\n"
        "del \"%s\"\n"
        "endlocal\n",
        current_pid, path, path, bat_path);

    fclose(fp);

    // ���� bat �ļ�
    SHELLEXECUTEINFOA sei = { sizeof(sei) };
    sei.lpFile = bat_path;
    sei.nShow = SW_HIDE;

    if (!ShellExecuteExA(&sei)) {
        bot_log("[-] Failed to start update script: %lu\n", GetLastError());
        DeleteFileA(bat_path); // ����ʧ��ʱ�Ĳ����ļ�
        return -1;
    }

    bot_log("[+] Update script launched. Exiting...\n");

    WSACleanup();
    ExitProcess(0); // ȷ����ǰ���̸ɾ��˳�
}


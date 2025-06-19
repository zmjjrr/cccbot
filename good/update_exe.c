#include"header.h"


int update_exe(char* path) {

    SHELLEXECUTEINFOA sei = { sizeof(sei) };
    sei.lpVerb = "open";
    sei.lpFile = path;
    if(DEBUG)
        sei.nShow = SW_SHOWNORMAL;
    else
        sei.nShow = SW_HIDE;

    if (!ShellExecuteExA(&sei)) {// ANSI version
        wprintf(L"[-] Failed to start update executable: %lu\n", GetLastError());
        return -1;
    }

    wprintf(L"[+] Successfully launched new version. Exiting...\n");

    WSACleanup();

    delete_self_with_batch();

    return 0;
}


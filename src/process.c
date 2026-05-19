#include "process.h"
#include "config.h"

void build_cmd(App *app, int side, int wink, char *out, int out_sz)
{
    const char *rom = app->fields[FIELD_ROM].buf;
    const char *ip = app->fields[FIELD_IP].buf;
    const char *port;
    const char *pport;
    if (side) {
        port = "7000";
        pport = "7001";
    }
    else {
        port = "7001";
        pport = "7000";
    }

    const char *win = wink ? "-w" : "";
    char ep[512];
    emu_path(app, ep, sizeof(ep));
#ifdef _WIN32
    snprintf(out, out_sz, "\"%s\" quark:direct,%s,%s,%s,%s,%d,0 %s",
             ep, rom, port, ip, pport, side, win);
#else
    snprintf(out, out_sz, "%s \"%s\" quark:direct,%s,%s,%s,%s,%d,0 %s",
             LAUNCHER, ep, rom, port, ip, pport, side, win);
#endif
}

void launch_emulator(App *app)
{
    char cmd[1024];
    int side = app->player;
    build_cmd(app, side, app->windowed, cmd, sizeof(cmd));

    app->output_len = snprintf(app->output, MAX_OUTPUT, "$ %s\n", cmd);
    app->launch_handle = INVALID_PROC;

#ifdef _WIN32
    STARTUPINFO si = {sizeof(si)};
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOW;
    if (CreateProcess(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        CloseHandle(pi.hThread);
        app->launch_handle = pi.hProcess;
        int l = strlen(app->output);
        snprintf(app->output + l, MAX_OUTPUT - l,
                 "Game launched (PID: %lu)\n", pi.dwProcessId);
        app->output_len = strlen(app->output);
        snprintf(app->status, sizeof(app->status),
                 "Running (PID: %lu)", pi.dwProcessId);
    }
    else
    {
        snprintf(app->status, sizeof(app->status),
                 "CreateProcess failed (error %lu)", GetLastError());
    }
#else
    pid_t pid = fork();
    if (pid == 0)
    {
        execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
        _exit(1);
    }
    else if (pid > 0)
    {
        app->launch_handle = pid;
        int l = strlen(app->output);
        snprintf(app->output + l, MAX_OUTPUT - l,
                 "Game launched (PID: %d)\n", pid);
        app->output_len = strlen(app->output);
        snprintf(app->status, sizeof(app->status),
                 "Running (PID: %d)", pid);
    }
    else
    {
        snprintf(app->status, sizeof(app->status), "fork() failed!");
    }
#endif
}

void check_child(App *app)
{
    if (app->launch_handle == INVALID_PROC)
        return;
#ifdef _WIN32
    DWORD code;
    if (!GetExitCodeProcess(app->launch_handle, &code))
        return;
    if (code == STILL_ACTIVE)
        return;
    CloseHandle(app->launch_handle);
    int len = strlen(app->output);
    snprintf(app->output + len, MAX_OUTPUT - len,
             "Exited with code %lu\n", (unsigned long)code);
    app->output_len = strlen(app->output);
    snprintf(app->status, sizeof(app->status),
             "Exited (code %lu)", (unsigned long)code);
    app->launch_handle = INVALID_PROC;
#else
    int wstatus;
    pid_t ret = waitpid(app->launch_handle, &wstatus, WNOHANG);
    if (ret != app->launch_handle)
        return;
    int len = strlen(app->output);
    if (WIFEXITED(wstatus))
    {
        snprintf(app->output + len, MAX_OUTPUT - len,
                 "Exited with code %d\n", WEXITSTATUS(wstatus));
        snprintf(app->status, sizeof(app->status),
                 "Exited (code %d)", WEXITSTATUS(wstatus));
    }
    else if (WIFSIGNALED(wstatus))
    {
        snprintf(app->output + len, MAX_OUTPUT - len,
                 "Killed by signal %d\n", WTERMSIG(wstatus));
        snprintf(app->status, sizeof(app->status),
                 "Killed (signal %d)", WTERMSIG(wstatus));
    }
    app->output_len = strlen(app->output);
    app->launch_handle = INVALID_PROC;
#endif
}

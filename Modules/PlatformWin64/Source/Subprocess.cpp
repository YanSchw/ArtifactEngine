#include "Platform/Subprocess.h"
#include <windows.h>

static void ReadPipeToString(HANDLE pipe, String& out) {
    char buffer[4096];
    DWORD bytesRead;

    while (ReadFile(pipe, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
        out.append(buffer, bytesRead);
    }
}

SubprocessResult Subprocess::Run(const String& InCommand) {
    SubprocessResult result;

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;

    HANDLE outRead = NULL, outWrite = NULL;
    HANDLE errRead = NULL, errWrite = NULL;

    if (!CreatePipe(&outRead, &outWrite, &sa, 0)) return result;
    if (!CreatePipe(&errRead, &errWrite, &sa, 0)) return result;

    SetHandleInformation(outRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(errRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si{};
    si.cb = sizeof(STARTUPINFOA);
    si.hStdOutput = outWrite;
    si.hStdError  = errWrite;
    si.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION pi{};

    String command = "cmd.exe /C " + InCommand;

    if (!CreateProcessA(
            NULL,
            command.data(),
            NULL,
            NULL,
            TRUE,
            0,
            NULL,
            NULL,
            &si,
            &pi))
    {
        return result;
    }

    CloseHandle(outWrite);
    CloseHandle(errWrite);

    // Read both pipes (simple sequential read; works for most cases)
    ReadPipeToString(outRead, result.StdOut);
    ReadPipeToString(errRead, result.StdErr);

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    result.ExitCode = static_cast<int>(exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(outRead);
    CloseHandle(errRead);

    return result;
}

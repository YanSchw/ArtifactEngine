#include "Platform/Subprocess.h"
#include <cstdio>
#include <sys/wait.h>

SubprocessResult Subprocess::Run(const String& InCommand) {
    SubprocessResult result;

    String tempFile = "/tmp/subprocess_stderr.txt";
    String fullCmd = InCommand + " 2> " + tempFile;

    FILE* pipe = popen(fullCmd.c_str(), "r");
    if (!pipe)
        return result;

    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result.StdOut += buffer;
    }

    int rc = pclose(pipe);

    if (WIFEXITED(rc))
        result.ExitCode = WEXITSTATUS(rc);
    else
        result.ExitCode = rc;

    FILE* errFile = fopen(tempFile.c_str(), "r");
    if (errFile) {
        while (fgets(buffer, sizeof(buffer), errFile)) {
            result.StdErr += buffer;
        }
        fclose(errFile);
        remove(tempFile.c_str());
    }

    return result;
}
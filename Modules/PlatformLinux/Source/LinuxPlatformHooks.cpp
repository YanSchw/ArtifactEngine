#include "LinuxPlatformHooks.h"
#include "CoreMinimal.h"
#include "Platform/Subprocess.h"

static constexpr int s_CommandNotFound = 127;

Array<String> LinuxPlatformHooks::OpenFilesDialog(const String& InTitle) {
    const Array<String> commands = {
        "zenity --file-selection --multiple --separator='|' --title='" + InTitle + "'",
        "kdialog --getopenfilename --multiple --separate-output --title '" + InTitle + "'",
    };

    Array<String> files;
    for (const String& command : commands) {
        const SubprocessResult result = Subprocess::Run(command);
        if (result.ExitCode == s_CommandNotFound) {
            continue;
        }
        String current;
        for (char c : result.StdOut) {
            if (c == '|' || c == '\n') {
                if (!current.empty()) {
                    files.Add(current);
                }
                current.clear();
            } else {
                current += c;
            }
        }
        if (!current.empty()) {
            files.Add(current);
        }
        return files;
    }

    AE_WARN("Picking files needs zenity or kdialog to be installed");
    return files;
}

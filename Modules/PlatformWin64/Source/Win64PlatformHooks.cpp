#include "Win64PlatformHooks.h"
#include "Win64FileDialog.h"

Array<String> Win64PlatformHooks::OpenFilesDialog(const String& InTitle) {
    Array<String> files;
    for (const std::filesystem::path& file : Win64OpenFilesDialog(std::wstring(InTitle.begin(), InTitle.end()))) {
        const std::u8string utf8 = file.u8string();
        files.Add(String(utf8.begin(), utf8.end()));
    }
    return files;
}

// This file must not include any engine header, for the same reason as Win64KnownFolders.cpp.
#include "Win64FileDialog.h"
#include <windows.h>
#include <commdlg.h>

#pragma comment(lib, "comdlg32.lib")

std::vector<std::filesystem::path> Win64OpenFilesDialog(const std::wstring& InTitle) {
    std::vector<wchar_t> buffer(64 * 1024, L'\0');
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = GetActiveWindow();
    dialog.lpstrFile = buffer.data();
    dialog.nMaxFile = (DWORD)buffer.size();
    dialog.lpstrTitle = InTitle.c_str();
    dialog.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    std::vector<std::filesystem::path> files;
    if (!GetOpenFileNameW(&dialog)) {
        return files;
    }

    // One pick is a full path; several are the directory followed by each file name.
    const std::filesystem::path first = buffer.data();
    for (const wchar_t* name = buffer.data() + first.native().size() + 1; *name; name += wcslen(name) + 1) {
        files.push_back(first / name);
    }
    if (files.empty()) {
        files.push_back(first);
    }
    return files;
}

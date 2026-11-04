// This file must not include any engine header. <shlobj.h> pulls in rpcdce.h, which declares a global
// `typedef GUID UUID` that collides with the engine's own UUID type (error C2371).
#include "Win64KnownFolders.h"
#include <windows.h>
#include <shlobj.h>

std::filesystem::path Win64GetRoamingAppDataDirectory() {
    PWSTR path = nullptr;
    std::filesystem::path directory;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path))) {
        directory = path;
    }
    CoTaskMemFree(path);
    return directory;
}
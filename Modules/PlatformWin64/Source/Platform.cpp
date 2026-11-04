#include "Platform/Platform.h"
#include <windows.h>
#include <shlobj.h>
#include <filesystem>

static std::filesystem::path GetExecutablePath() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return std::filesystem::path(buffer);
}

PlatformType Platform::CurrentPlatform() {
    return PlatformType::Win64;
}

Class Platform::GetDefaultRenderingAPIClass() {
    return Class("VulkanAPI");
}

Class Platform::GetMainSurfaceClass() {
    return Class("Window");
}

String Platform::GetResourceDirectory() {
    return GetExecutablePath().parent_path().string();
}

String Platform::GetContentDirectory() {
    return (std::filesystem::path(GetResourceDirectory()) / "Content").string();
}

String Platform::GetUserConfigDirectory() {
    PWSTR path = nullptr;
    std::filesystem::path directory;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path))) {
        directory = path;
    }
    CoTaskMemFree(path);
    return directory.string();
}

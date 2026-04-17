#include "Platform/Platform.h"
#include <windows.h>
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

String Platform::GetContentDirectory() {
    auto exePath = GetExecutablePath();
    auto baseDir = exePath.parent_path();

    return (baseDir / "Content").string();
}
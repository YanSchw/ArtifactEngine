#include "Platform/Platform.h"
#include <unistd.h>
#include <limits.h>

static std::filesystem::path GetExecutablePath() {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count <= 0)
        return {};

    return std::filesystem::path(std::string(result, count));
}

PlatformType Platform::CurrentPlatform() {
    return PlatformType::Linux;
}

Class Platform::GetDefaultRenderingAPIClass() {
    return Class("VulkanAPI");
}

String Platform::GetContentDirectory() {
    auto exePath = GetExecutablePath();
    auto baseDir = exePath.parent_path();

    return (baseDir / "Content").string();
}
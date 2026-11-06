#include "Platform/Platform.h"
#include <unistd.h>
#include <limits.h>
#include <pwd.h>
#include <cstdlib>

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

Class Platform::GetDefaultAudioDeviceClass() {
    return Class("AlsaAudioDevice");
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

static std::filesystem::path GetHomeDirectory() {
    if (const char* home = std::getenv("HOME"); home && *home) {
        return home;
    }
    const passwd* entry = getpwuid(getuid());
    return entry ? entry->pw_dir : "";
}

String Platform::GetUserConfigDirectory() {
    if (const char* configHome = std::getenv("XDG_CONFIG_HOME"); configHome && *configHome) {
        return configHome;
    }
    return (GetHomeDirectory() / ".config").string();
}

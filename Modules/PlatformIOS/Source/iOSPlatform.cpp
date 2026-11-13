#include "Platform/Platform.h"
#include "Platform/Subprocess.h"
#include "iOSBridge.h"

#include <CoreFoundation/CoreFoundation.h>
#include <climits>
#include <cstdlib>
#include <filesystem>

int ArtifactMain(const Array<String>& InArgs);

static std::filesystem::path GetBundleResourcesPath() {
    CFURLRef url = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
    if (!url) {
        return {};
    }
    char path[PATH_MAX];
    const bool resolved = CFURLGetFileSystemRepresentation(url, true, (UInt8*)path, PATH_MAX);
    CFRelease(url);
    return resolved ? std::filesystem::path(path) : std::filesystem::path();
}

PlatformType Platform::CurrentPlatform() {
    return PlatformType::iOS;
}

Class Platform::GetDefaultRenderingAPIClass() {
    return Class("VulkanAPI");
}

Class Platform::GetDefaultAudioDeviceClass() {
    return Class("iOSAudioDevice");
}

Class Platform::GetMainSurfaceClass() {
    return Class("iOSSurface");
}

String Platform::GetResourceDirectory() {
    const std::filesystem::path resources = GetBundleResourcesPath();
    AE_ASSERT(!resources.empty(), "Failed to resolve the app bundle's resource directory");
    return resources.empty() ? "." : resources.string();
}

String Platform::GetContentDirectory() {
    return (std::filesystem::path(GetResourceDirectory()) / "Content").string();
}

String Platform::GetUserConfigDirectory() {
    const char* home = std::getenv("HOME");
    return (std::filesystem::path(home ? home : ".") / "Library" / "Application Support").string();
}

SubprocessResult Subprocess::Run(const String& InCommand) {
    AE_WARN("Cannot run '{0}': iOS apps cannot spawn subprocesses", InCommand);
    return SubprocessResult{ -1, "", "iOS apps cannot spawn subprocesses" };
}

int iOSBridge::RunEngine(int InArgc, char** InArgv) {
    Array<String> args;
    for (int i = 0; i < InArgc; i++) {
        args.Add(String(InArgv[i]));
    }
    return ArtifactMain(args);
}

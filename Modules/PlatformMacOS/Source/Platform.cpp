#include "Platform/Platform.h"

#include <CoreFoundation/CoreFoundation.h>

static std::filesystem::path GetMacOSResourcesPath() {
    CFBundleRef bundle = CFBundleGetMainBundle();
    if (!bundle)
        return {};

    CFURLRef url = CFBundleCopyResourcesDirectoryURL(bundle);
    if (!url)
        return {};

    char path[PATH_MAX];
    bool ok = CFURLGetFileSystemRepresentation(url, true, (UInt8*)path, PATH_MAX);

    CFRelease(url);

    if (!ok)
        return {};

    return std::filesystem::path(path);
}

PlatformType Platform::CurrentPlatform() {
    return PlatformType::MacOS;
}

Class Platform::GetDefaultRenderingAPIClass() {
    return Class("VulkanAPI");
}

String Platform::GetResourceDirectory() {
    auto resources = GetMacOSResourcesPath();
    if (resources.empty()) {
        AE_ASSERT(false, "Failed to get macOS resources path");
        return ".";
    }

    return resources.string();
}

String Platform::GetContentDirectory() {
    return (std::filesystem::path(GetResourceDirectory()) / "Content").string();
}
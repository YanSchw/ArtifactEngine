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

// AppKit is reached through the raw Objective-C runtime: its headers typedef `Class`,
// which collides with the engine's reflection type of the same name.
extern "C" {
    void* objc_getClass(const char* InName);
    void* sel_registerName(const char* InName);
    void objc_msgSend(void);
}

static void* SendMessage(void* InReceiver, const char* InSelector) {
    return ((void* (*)(void*, void*))objc_msgSend)(InReceiver, sel_registerName(InSelector));
}

static void* SendMessage(void* InReceiver, const char* InSelector, void* InArg) {
    return ((void* (*)(void*, void*, void*))objc_msgSend)(InReceiver, sel_registerName(InSelector), InArg);
}

void Platform::SetApplicationIcon(const String& InImagePath) {
    void* path = ((void* (*)(void*, void*, const char*))objc_msgSend)(
        objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), InImagePath.c_str());
    void* image = SendMessage(SendMessage(objc_getClass("NSImage"), "alloc"), "initWithContentsOfFile:", path);
    if (!image) {
        AE_WARN("Failed to load application icon '{0}'", InImagePath);
        return;
    }

    void* app = SendMessage(objc_getClass("NSApplication"), "sharedApplication");
    SendMessage(app, "setApplicationIconImage:", image);
    SendMessage(image, "release");
}
#pragma once
#include "CoreMinimal.h"
#include "Platform.gen.h"

ARTIFACT_ENUM();
enum class PlatformType : uint32_t {
    Unknown = 0,
    Win64,
    MacOS,
    Linux,
};

class Platform {
public:

    static PlatformType CurrentPlatform();

    static Class GetDefaultRenderingAPIClass();

    // Directory holding the application's bundled resources. In packaged builds this is the
    // platform's resource location (e.g. Contents/Resources inside a macOS .app); otherwise it is
    // the directory next to the executable. GetContentDirectory() resolves relative to this.
    static String GetResourceDirectory();
    static String GetContentDirectory();

    // Icon the OS shows for the running process.
    static void SetApplicationIcon(const String& InImagePath);

    struct TemporaryDirectory {
        String Path;
        TemporaryDirectory();
        ~TemporaryDirectory();
    };

};
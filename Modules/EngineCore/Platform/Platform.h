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
    static String GetContentDirectory();

    struct TemporaryDirectory {
        String Path;
        TemporaryDirectory();
        ~TemporaryDirectory();
    };

};
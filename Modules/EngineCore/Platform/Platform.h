#pragma once
#include "CoreMinimal.h"

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

    struct TemporaryDirectory {
        String Path;
        TemporaryDirectory();
        ~TemporaryDirectory();
    };

};
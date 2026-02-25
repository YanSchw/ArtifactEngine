#pragma once
#include "Common/String.h"

enum class PlatformType : uint32_t {
    Unknown = 0,
    Win64,
    MacOS,
    Linux,
};

class Platform {
public:
    
    static PlatformType CurrentPlatform();

};
#include "Core/Platform.h"

PlatformType Platform::CurrentPlatform() {
    return PlatformType::Linux;
}

Class Platform::GetDefaultRenderingAPIClass() {
    return Class("VulkanAPI");
}
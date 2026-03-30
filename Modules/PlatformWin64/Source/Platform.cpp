#include "Core/Platform.h"

PlatformType Platform::CurrentPlatform() {
    return PlatformType::Win64;
}

Class Platform::GetDefaultRenderingAPIClass() {
    return Class("VulkanAPI");
}
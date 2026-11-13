#define VK_USE_PLATFORM_METAL_EXT
#include "VulkanPresentation.h"
#include "iOSSurface.h"

std::vector<const char*> GetPresentationInstanceExtensions() {
    return { VK_KHR_SURFACE_EXTENSION_NAME, VK_EXT_METAL_SURFACE_EXTENSION_NAME };
}

VkResult CreatePresentationSurface(VkInstance InInstance, Surface* InSurface, VkSurfaceKHR* OutSurface) {
    iOSSurface* surface = InSurface ? InSurface->As<iOSSurface>() : nullptr;
    AE_ASSERT(surface, "surface render targets must be iOS surfaces");

    VkMetalSurfaceCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
    createInfo.pLayer = (const CAMetalLayer*)surface->GetMetalLayer();
    return vkCreateMetalSurfaceEXT(InInstance, &createInfo, nullptr, OutSurface);
}

#include "VulkanPresentation.h"
#include "Window.h"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

std::vector<const char*> GetPresentationInstanceExtensions() {
    uint32_t count = 0;
    const char** names = glfwGetRequiredInstanceExtensions(&count);
    return std::vector<const char*>(names, names + count);
}

VkResult CreatePresentationSurface(VkInstance InInstance, Surface* InSurface, VkSurfaceKHR* OutSurface) {
    Window* window = InSurface ? InSurface->As<Window>() : nullptr;
    AE_ASSERT(window, "surface render targets must be windows");
    return glfwCreateWindowSurface(InInstance, window->GetNativeWindow(), nullptr, OutSurface);
}

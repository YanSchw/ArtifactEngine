#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class Surface;

std::vector<const char*> GetPresentationInstanceExtensions();
VkResult CreatePresentationSurface(VkInstance InInstance, Surface* InSurface, VkSurfaceKHR* OutSurface);

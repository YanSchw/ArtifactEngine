#pragma once
#include "Rendering/Image.h"
#include <vulkan/vulkan.h>

class VulkanHelpers {
public:
    static void TransitionImage(VkCommandBuffer InCmd, VkImage InImage, VkImageLayout InOldLayout, VkImageLayout InNewLayout, VkImageAspectFlags InAspectMask);
    static VkFormat ImageFormatToVkFormat(ImageFormat format);
    static VkImageUsageFlags ImageUsageToVkImageUsage(ImageUsage usage);
};
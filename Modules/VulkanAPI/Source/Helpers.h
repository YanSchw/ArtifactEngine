#pragma once
#include <vulkan/vulkan.h>

class VulkanHelpers {
public:
    static void TransitionImage(VkCommandBuffer InCmd, VkImage InImage, VkImageLayout InOldLayout, VkImageLayout InNewLayout, VkImageAspectFlags InAspectMask);
};
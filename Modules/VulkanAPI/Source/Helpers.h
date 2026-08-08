#pragma once
#include "Rendering/Image.h"
#include <vulkan/vulkan.h>

class VulkanHelpers {
public:
    static void TransitionImage(VkCommandBuffer InCmd, VkImage InImage, VkImageLayout InOldLayout, VkImageLayout InNewLayout, VkImageAspectFlags InAspectMask, uint32_t InBaseLayer = 0, uint32_t InLayerCount = 1);
    static VkFormat ImageFormatToVkFormat(ImageFormat format);
    static VkImageUsageFlags ImageUsageToVkImageUsage(ImageUsage usage);
    static VkSampleCountFlagBits SampleCountToVkSampleCount(SampleCount samples);
};
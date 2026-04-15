#include "Helpers.h"
#include "VulkanAPI.h"

struct ImageTransitionInfo {
    VkPipelineStageFlags srcStage;
    VkPipelineStageFlags dstStage;
    VkAccessFlags srcAccess;
    VkAccessFlags dstAccess;
};

static ImageTransitionInfo GetTransitionInfo(
    VkImageLayout oldLayout,
    VkImageLayout newLayout)
{
    // Default (safe fallback)
    ImageTransitionInfo info{};
    info.srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    info.dstStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    info.srcAccess = 0;
    info.dstAccess = 0;

    // --- Common transitions ---

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        info.dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        info.dstAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
             newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
    {
        info.dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        info.dstAccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        info.srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        info.srcAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        info.dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        info.dstAccess = VK_ACCESS_SHADER_READ_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR &&
             newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        info.srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        info.dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        info.dstAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
    {
        info.srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        info.srcAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        info.dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    }

    return info;
}

void VulkanHelpers::TransitionImage(VkCommandBuffer InCmd, VkImage InImage, VkImageLayout InOldLayout, VkImageLayout InNewLayout, VkImageAspectFlags InAspectMask) {
    if (InOldLayout == InNewLayout)
        return; // no-op

    ImageTransitionInfo t = GetTransitionInfo(InOldLayout, InNewLayout);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

    barrier.oldLayout = InOldLayout;
    barrier.newLayout = InNewLayout;

    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = InImage;

    barrier.subresourceRange = {
        InAspectMask,
        0, 1,
        0, 1
    };

    barrier.srcAccessMask = t.srcAccess;
    barrier.dstAccessMask = t.dstAccess;

    vkCmdPipelineBarrier(
        InCmd,
        t.srcStage,
        t.dstStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
}

VkFormat VulkanHelpers::ImageFormatToVkFormat(ImageFormat format) {
    switch (format) {
    case ImageFormat::RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;
    case ImageFormat::BGRA8: return VK_FORMAT_B8G8R8A8_UNORM;
    case ImageFormat::RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
    case ImageFormat::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
    case ImageFormat::Depth24Stencil8: return VK_FORMAT_D24_UNORM_S8_UINT;
    case ImageFormat::Depth32F: return VK_FORMAT_D32_SFLOAT;
    default: return VK_FORMAT_UNDEFINED;
    }
}

VkImageUsageFlags VulkanHelpers::ImageUsageToVkImageUsage(ImageUsage usage) {
    VkImageUsageFlags flags = 0;
    if ((usage & ImageUsage::TransferSrc) != ImageUsage::None) flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if ((usage & ImageUsage::TransferDst) != ImageUsage::None) flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if ((usage & ImageUsage::Sampled) != ImageUsage::None) flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if ((usage & ImageUsage::Storage) != ImageUsage::None) flags |= VK_IMAGE_USAGE_STORAGE_BIT;
    if ((usage & ImageUsage::ColorAttachment) != ImageUsage::None) flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if ((usage & ImageUsage::DepthStencil) != ImageUsage::None) flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    return flags;
}
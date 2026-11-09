#include "Helpers.h"
#include "VulkanAPI.h"

struct LayoutAccess {
    VkPipelineStageFlags Stages;
    VkAccessFlags Access;
};

static constexpr VkPipelineStageFlags s_ShaderStages =
    VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

static LayoutAccess GetLayoutAccess(VkImageLayout InLayout, bool InIsSource) {
    switch (InLayout) {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            return { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0 };
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT };
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
            return { VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT };
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return { s_ShaderStages, InIsSource ? 0u : (VkAccessFlags)VK_ACCESS_SHADER_READ_BIT };
        case VK_IMAGE_LAYOUT_GENERAL:
            return { s_ShaderStages, InIsSource ? (VkAccessFlags)VK_ACCESS_SHADER_WRITE_BIT : (VkAccessFlags)(VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT) };
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            return { VK_PIPELINE_STAGE_TRANSFER_BIT, InIsSource ? 0u : (VkAccessFlags)VK_ACCESS_TRANSFER_READ_BIT };
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            return { VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT };
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            return { (VkPipelineStageFlags)(InIsSource ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT : VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT), 0 };
        default:
            return { VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT };
    }
}

void VulkanHelpers::TransitionImage(VkCommandBuffer InCmd, VkImage InImage, VkImageLayout InOldLayout, VkImageLayout InNewLayout, VkImageAspectFlags InAspectMask,
                                    uint32_t InBaseLayer, uint32_t InLayerCount, uint32_t InBaseMip, uint32_t InMipCount) {
    if (InOldLayout == InNewLayout)
        return;

    const LayoutAccess src = GetLayoutAccess(InOldLayout, true);
    const LayoutAccess dst = GetLayoutAccess(InNewLayout, false);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = InOldLayout;
    barrier.newLayout = InNewLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = InImage;
    barrier.subresourceRange = { InAspectMask, InBaseMip, InMipCount, InBaseLayer, InLayerCount };
    barrier.srcAccessMask = src.Access;
    barrier.dstAccessMask = dst.Access;

    vkCmdPipelineBarrier(InCmd, src.Stages, dst.Stages, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void VulkanHelpers::GlobalBarrier(VkCommandBuffer InCmd, VkPipelineStageFlags InSrcStages, VkAccessFlags InSrcAccess, VkPipelineStageFlags InDstStages, VkAccessFlags InDstAccess) {
    VkMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.srcAccessMask = InSrcAccess;
    barrier.dstAccessMask = InDstAccess;

    vkCmdPipelineBarrier(InCmd, InSrcStages, InDstStages, 0, 1, &barrier, 0, nullptr, 0, nullptr);
}

VkFormat VulkanHelpers::ImageFormatToVkFormat(ImageFormat format) {
    switch (format) {
    case ImageFormat::RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;
    case ImageFormat::BGRA8: return VK_FORMAT_B8G8R8A8_UNORM;
    case ImageFormat::RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
    case ImageFormat::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
    case ImageFormat::R32UI: return VK_FORMAT_R32_UINT;
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
    if ((usage & ImageUsage::Transient) != ImageUsage::None) flags |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
    return flags;
}

VkSampleCountFlagBits VulkanHelpers::SampleCountToVkSampleCount(SampleCount samples) {
    switch (samples) {
    case SampleCount::X2: return VK_SAMPLE_COUNT_2_BIT;
    case SampleCount::X4: return VK_SAMPLE_COUNT_4_BIT;
    case SampleCount::X8: return VK_SAMPLE_COUNT_8_BIT;
    default: return VK_SAMPLE_COUNT_1_BIT;
    }
}

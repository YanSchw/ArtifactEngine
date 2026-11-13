#include "VulkanImage.h"
#include "VulkanAPI.h"
#include "Helpers.h"
#include <stdexcept>

Array<VulkanImage*> s_Images;
Array<VulkanImageView*> s_ImageViews;

VulkanImage::VulkanImage(const ImageDesc& InImageDesc, VulkanAPI& InVulkanAPI) {
    s_Images.Add(this);
    m_Desc = InImageDesc;
    m_VulkanAPI = &InVulkanAPI;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = InImageDesc.Width;
    imageInfo.extent.height = InImageDesc.Height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = InImageDesc.MipLevels;
    imageInfo.arrayLayers = InImageDesc.ArrayLayers;
    imageInfo.format = VulkanHelpers::ImageFormatToVkFormat(InImageDesc.Format);
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VulkanHelpers::ImageUsageToVkImageUsage(InImageDesc.Usage);
    imageInfo.samples = VulkanHelpers::SampleCountToVkSampleCount(InImageDesc.Samples);
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(m_VulkanAPI->GetDevice(), &imageInfo, nullptr, &m_Image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_VulkanAPI->GetDevice(), m_Image, &memRequirements);

    // Transient images never leave the render pass, so prefer memory the driver may keep on-chip
    uint32_t memoryTypeIndex = 0;
    const bool isTransient = (InImageDesc.Usage & ImageUsage::Transient) != ImageUsage::None;
    if (!isTransient || !m_VulkanAPI->TryFindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, memoryTypeIndex)) {
        memoryTypeIndex = m_VulkanAPI->FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    if (vkAllocateMemory(m_VulkanAPI->GetDevice(), &allocInfo, nullptr, &m_DeviceMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(m_VulkanAPI->GetDevice(), m_Image, m_DeviceMemory, 0);

    if (HasUsage(ImageUsage::Storage)) {
        m_VulkanAPI->ExecuteImmediate([this](VkCommandBuffer InCmdBuffer) {
            VulkanHelpers::TransitionImage(InCmdBuffer, m_Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT,
                0, VK_REMAINING_ARRAY_LAYERS, 0, VK_REMAINING_MIP_LEVELS);
        });
    }
}

VkImageLayout VulkanImage::GetShaderReadLayout() const {
    return HasUsage(ImageUsage::Storage) ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

VulkanImage::~VulkanImage() {
    s_Images.Remove(this);

    if (m_DeviceMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_VulkanAPI->GetDevice(), m_DeviceMemory, nullptr);
    }
    if (m_Image != VK_NULL_HANDLE) {
        vkDestroyImage(m_VulkanAPI->GetDevice(), m_Image, nullptr);
    }
}


static VkImageViewType ImageViewTypeToVkImageViewType(ImageViewType type) {
    switch (type) {
    case ImageViewType::Type2D: return VK_IMAGE_VIEW_TYPE_2D;
    case ImageViewType::Type2DArray: return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    case ImageViewType::Cube: return VK_IMAGE_VIEW_TYPE_CUBE;
    case ImageViewType::CubeArray: return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
    default: return VK_IMAGE_VIEW_TYPE_2D;
    }
}

static VkImageAspectFlags ImageAspectToVkImageAspect(ImageAspect aspect) {
    switch (aspect) {
    case ImageAspect::Color: return VK_IMAGE_ASPECT_COLOR_BIT;
    case ImageAspect::Depth: return VK_IMAGE_ASPECT_DEPTH_BIT;
    case ImageAspect::Stencil: return VK_IMAGE_ASPECT_STENCIL_BIT;
    case ImageAspect::DepthStencil: return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    default: return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}

static constexpr VkImageUsageFlags s_AttachmentUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                                                       | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;

static uint32_t GetMaxFramebufferLayers(VulkanAPI& InVulkanAPI) {
    static const uint32_t s_MaxLayers = [&InVulkanAPI] {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(InVulkanAPI.GetPhysicalDevice(), &properties);
        return properties.limits.maxFramebufferLayers;
    }();
    return s_MaxLayers;
}

VulkanImageView::VulkanImageView(const ImageViewDesc& InImageViewDesc, VulkanAPI& InVulkanAPI) {
    s_ImageViews.Add(this);
    m_Desc = InImageViewDesc;
    m_VulkanAPI = &InVulkanAPI;

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = static_cast<VulkanImage*>(InImageViewDesc.ImagePtr.Get())->GetVkImage();
    viewInfo.viewType = ImageViewTypeToVkImageViewType(InImageViewDesc.ViewType);
    viewInfo.format = VulkanHelpers::ImageFormatToVkFormat(InImageViewDesc.Format);
    viewInfo.subresourceRange.aspectMask = ImageAspectToVkImageAspect(InImageViewDesc.Aspect);
    viewInfo.subresourceRange.baseMipLevel = InImageViewDesc.BaseMip;
    viewInfo.subresourceRange.levelCount = InImageViewDesc.MipCount;
    viewInfo.subresourceRange.baseArrayLayer = InImageViewDesc.BaseLayer;
    viewInfo.subresourceRange.layerCount = InImageViewDesc.LayerCount;

    VkImageViewUsageCreateInfo usageInfo{};
    usageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
    usageInfo.usage = VulkanHelpers::ImageUsageToVkImageUsage(InImageViewDesc.ImagePtr->GetDesc().Usage) & ~s_AttachmentUsage;
    if (InImageViewDesc.LayerCount > GetMaxFramebufferLayers(*m_VulkanAPI) && usageInfo.usage != 0) {
        viewInfo.pNext = &usageInfo;
    }

    if (vkCreateImageView(m_VulkanAPI->GetDevice(), &viewInfo, nullptr, &m_ImageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }
}

VulkanImage* VulkanImageView::GetImage() const {
    return m_Desc.ImagePtr ? m_Desc.ImagePtr->As<VulkanImage>() : nullptr;
}

VulkanImageView::~VulkanImageView() {
    s_ImageViews.Remove(this);

    if (m_ImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_VulkanAPI->GetDevice(), m_ImageView, nullptr);
    }
}

void VulkanImage::DestroyAll() {
    Array<VulkanImage*> imagesToDestroy = s_Images;
    for (VulkanImage* image : imagesToDestroy) {
        delete image;
    }
    s_Images.Clear();
}

void VulkanImageView::DestroyAll() {
    Array<VulkanImageView*> imageViewsToDestroy = s_ImageViews;
    for (VulkanImageView* imageView : imageViewsToDestroy) {
        delete imageView;
    }
    s_ImageViews.Clear();
}

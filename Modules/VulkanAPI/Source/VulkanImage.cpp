#include "VulkanImage.h"
#include "VulkanAPI.h"
#include <stdexcept>

static VkFormat ImageFormatToVkFormat(ImageFormat format) {
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

static VkImageUsageFlags ImageUsageToVkImageUsage(ImageUsage usage) {
    VkImageUsageFlags flags = 0;
    if ((usage & ImageUsage::TransferSrc) != ImageUsage::None) flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if ((usage & ImageUsage::TransferDst) != ImageUsage::None) flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if ((usage & ImageUsage::Sampled) != ImageUsage::None) flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if ((usage & ImageUsage::Storage) != ImageUsage::None) flags |= VK_IMAGE_USAGE_STORAGE_BIT;
    if ((usage & ImageUsage::ColorAttachment) != ImageUsage::None) flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if ((usage & ImageUsage::DepthStencil) != ImageUsage::None) flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    return flags;
}

VulkanImage::VulkanImage(const ImageDesc& InImageDesc, VulkanAPI& InVulkanAPI) {
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
    imageInfo.format = ImageFormatToVkFormat(InImageDesc.Format);
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = ImageUsageToVkImageUsage(InImageDesc.Usage);
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(m_VulkanAPI->GetDevice(), &imageInfo, nullptr, &m_Image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_VulkanAPI->GetDevice(), m_Image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = m_VulkanAPI->FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(m_VulkanAPI->GetDevice(), &allocInfo, nullptr, &m_DeviceMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(m_VulkanAPI->GetDevice(), m_Image, m_DeviceMemory, 0);
}

VulkanImage::~VulkanImage() {
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

VulkanImageView::VulkanImageView(const ImageViewDesc& InImageViewDesc, VulkanAPI& InVulkanAPI) {
    m_Desc = InImageViewDesc;
    m_VulkanAPI = &InVulkanAPI;

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = static_cast<VulkanImage*>(InImageViewDesc.Image.Get())->GetVkImage();
    viewInfo.viewType = ImageViewTypeToVkImageViewType(InImageViewDesc.ViewType);
    viewInfo.format = ImageFormatToVkFormat(InImageViewDesc.Format);
    viewInfo.subresourceRange.aspectMask = ImageAspectToVkImageAspect(InImageViewDesc.Aspect);
    viewInfo.subresourceRange.baseMipLevel = InImageViewDesc.BaseMip;
    viewInfo.subresourceRange.levelCount = InImageViewDesc.MipCount;
    viewInfo.subresourceRange.baseArrayLayer = InImageViewDesc.BaseLayer;
    viewInfo.subresourceRange.layerCount = InImageViewDesc.LayerCount;

    if (vkCreateImageView(m_VulkanAPI->GetDevice(), &viewInfo, nullptr, &m_ImageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }
}

VulkanImageView::~VulkanImageView() {
    if (m_ImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_VulkanAPI->GetDevice(), m_ImageView, nullptr);
    }
}
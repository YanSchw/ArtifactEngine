#pragma once
#include "Rendering/Image.h"
#include "VulkanImage.gen.h"
#include "VulkanAPI.h"

#include <vulkan/vulkan.h>

class VulkanImage : public Image {
public:
    ARTIFACT_CLASS();

    VulkanImage(const ImageDesc& InImageDesc, VulkanAPI& InVulkanAPI);
    virtual ~VulkanImage();

    VkImage GetVkImage() const { return m_Image; }
    VkDeviceMemory GetVkDeviceMemory() const { return m_DeviceMemory; }

    static void DestroyAll();

private:
    VulkanAPI* m_VulkanAPI = nullptr;
    VkImage m_Image = VK_NULL_HANDLE;
    VkDeviceMemory m_DeviceMemory = VK_NULL_HANDLE;
};


class VulkanImageView : public ImageView {
public:
    ARTIFACT_CLASS();

    VulkanImageView(const ImageViewDesc& InImageViewDesc, VulkanAPI& InVulkanAPI);
    virtual ~VulkanImageView();

    VkImageView GetVkImageView() const { return m_ImageView; }

    static void DestroyAll();
    
private:
    VulkanAPI* m_VulkanAPI = nullptr;
    VkImageView m_ImageView = VK_NULL_HANDLE;
};
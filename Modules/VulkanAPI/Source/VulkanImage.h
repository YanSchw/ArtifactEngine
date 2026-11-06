#pragma once
#include "Rendering/Image.h"
#include "VulkanAPI.h"

#include <vulkan/vulkan.h>
#include "VulkanImage.gen.h"

class VulkanImage : public Image {
public:
    ARTIFACT_CLASS();

    VulkanImage(const ImageDesc& InImageDesc, VulkanAPI& InVulkanAPI);
    virtual ~VulkanImage();

    VkImage GetVkImage() const { return m_Image; }
    VkDeviceMemory GetVkDeviceMemory() const { return m_DeviceMemory; }

    /** The layout this image rests in whenever no pass is writing it as an attachment. Storage images
     *  stay in GENERAL so compute can write them without a transition. */
    VkImageLayout GetShaderReadLayout() const;

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
    VulkanImage* GetImage() const;

    static void DestroyAll();

private:
    VulkanAPI* m_VulkanAPI = nullptr;
    VkImageView m_ImageView = VK_NULL_HANDLE;
};
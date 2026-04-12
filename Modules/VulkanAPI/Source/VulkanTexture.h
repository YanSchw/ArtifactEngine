#pragma once
#include "Rendering/Texture.h"
#include "VulkanTexture.gen.h"
#include "VulkanAPI.h"

#include <vulkan/vulkan.h>

class VulkanTexture : public Texture {
public:
    ARTIFACT_CLASS();

    VulkanTexture(const String& InFilePath, const TextureDesc& InTextureDesc, VulkanAPI& InVulkanAPI);
    virtual ~VulkanTexture();

    virtual SharedObjectPtr<Image> GetImage() const override;
    virtual SharedObjectPtr<ImageView> GetDefaultView() const override;

    static void DestroyAll();

private:
    VulkanAPI* m_VulkanAPI = nullptr;
    SharedObjectPtr<Image> m_Image;
    SharedObjectPtr<ImageView> m_DefaultView;

    // Helper functions for Vulkan operations
    VkCommandBuffer BeginSingleTimeCommands(VulkanAPI& vulkanAPI);
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer, VulkanAPI& vulkanAPI);
    void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer commandBuffer);
};
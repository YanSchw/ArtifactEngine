#pragma once
#include "Rendering/FrameBuffer.h"
#include "VulkanFrameBuffer.gen.h"
#include "VulkanAPI.h"

#include <vulkan/vulkan.h>
#include <vector>

class VulkanFrameBuffer : public FrameBuffer {
public:
    ARTIFACT_CLASS();

    VulkanFrameBuffer(const FrameBufferDesc& InFrameBufferDesc, VulkanAPI& InVulkanAPI);
    virtual ~VulkanFrameBuffer() = default;

    const std::vector<VkImageView>& GetColorAttachmentViews() const { return m_ColorAttachmentViews; }
    VkImageView GetDepthAttachmentView() const { return m_DepthAttachmentView; }

private:
    VulkanAPI* m_VulkanAPI = nullptr;
    std::vector<VkImageView> m_ColorAttachmentViews;
    VkImageView m_DepthAttachmentView = VK_NULL_HANDLE;
};
#pragma once
#include "Rendering/FrameBuffer.h"
#include "VulkanAPI.h"

#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanFrameBuffer.gen.h"

class VulkanFrameBuffer : public FrameBuffer {
public:
    ARTIFACT_CLASS();

    VulkanFrameBuffer(const FrameBufferDesc& InFrameBufferDesc, VulkanAPI& InVulkanAPI);
    virtual ~VulkanFrameBuffer() = default;

    const std::vector<VkImageView>& GetColorAttachmentViews() const { return m_ColorAttachmentViews; }
    VkImageView GetDepthAttachmentView() const { return m_DepthAttachmentView; }

    uint32_t GetColorAttachmentCount() const { return (uint32_t)m_ColorAttachmentViews.size(); }

    std::vector<VkRenderingAttachmentInfo> GetColorAttachmentInfo() const;
    VkRenderingAttachmentInfo GetDepthAttachmentInfo() const;

    virtual uint32_t ReadPixelUint(int32_t InAttachment, uint32_t InX, uint32_t InY) const override;
private:
    VulkanAPI* m_VulkanAPI = nullptr;
    std::vector<VkImageView> m_ColorAttachmentViews;
    VkImageView m_DepthAttachmentView = VK_NULL_HANDLE;
};
#include "VulkanFrameBuffer.h"
#include "VulkanImage.h"

VulkanFrameBuffer::VulkanFrameBuffer(const FrameBufferDesc& InFrameBufferDesc, VulkanAPI& InVulkanAPI) {
    m_Desc = InFrameBufferDesc;
    m_VulkanAPI = &InVulkanAPI;

    // With dynamic rendering, we render directly to VkImageView attachments instead of creating a VkFramebuffer.
    for (auto& colorAttachment : InFrameBufferDesc.ColorAttachments) {
        m_ColorAttachmentViews.push_back(static_cast<VulkanImageView*>(colorAttachment.Get())->GetVkImageView());
    }

    if (InFrameBufferDesc.DepthAttachment) {
        m_DepthAttachmentView = static_cast<VulkanImageView*>(InFrameBufferDesc.DepthAttachment.Get())->GetVkImageView();
    }
}
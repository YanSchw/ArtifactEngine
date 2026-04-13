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

std::vector<VkRenderingAttachmentInfo> VulkanFrameBuffer::GetColorAttachmentInfo() const {
    std::vector<VkRenderingAttachmentInfo> attachmentInfos;
    for (VkImageView colorAttachmentView : m_ColorAttachmentViews) {
        VkRenderingAttachmentInfo attachmentInfo = {};
        attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        attachmentInfo.imageView = colorAttachmentView;
        attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentInfo.clearValue.color = { 0.1f, 0.1f, 0.1f, 1.0f };
        attachmentInfos.push_back(attachmentInfo);
    }
    return attachmentInfos;
}

VkRenderingAttachmentInfo VulkanFrameBuffer::GetDepthAttachmentInfo() const {
    VkRenderingAttachmentInfo attachmentInfo{};
    attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    attachmentInfo.imageView = m_DepthAttachmentView;
    attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    attachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentInfo.clearValue.depthStencil = {1.0f, 0};

    return attachmentInfo;
}
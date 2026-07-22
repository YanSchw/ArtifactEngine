#include "VulkanFrameBuffer.h"
#include "VulkanImage.h"
#include "Helpers.h"

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
    for (size_t i = 0; i < m_ColorAttachmentViews.size(); i++) {
        const Vec4 clear = m_Desc.GetClearColor((int32_t)i);
        const bool isInteger = m_Desc.ColorAttachments[(int32_t)i]->GetDesc().Format == ImageFormat::R32UI;

        VkRenderingAttachmentInfo attachmentInfo = {};
        attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        attachmentInfo.imageView = m_ColorAttachmentViews[i];
        attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        if (isInteger) {
            attachmentInfo.clearValue.color.uint32[0] = (uint32_t)clear.r;
        } else {
            attachmentInfo.clearValue.color = { clear.r, clear.g, clear.b, clear.a };
        }
        attachmentInfos.push_back(attachmentInfo);
    }
    return attachmentInfos;
}

uint32_t VulkanFrameBuffer::ReadPixelUint(int32_t InAttachment, uint32_t InX, uint32_t InY) const {
    if (InAttachment < 0 || InAttachment >= m_Desc.ColorAttachments.Size()) {
        return 0;
    }
    if (InX >= m_Desc.Width || InY >= m_Desc.Height) {
        return 0;
    }
    VulkanImage* image = m_Desc.ColorAttachments[InAttachment]->GetDesc().ImagePtr->As<VulkanImage>();
    if (!image) {
        return 0;
    }

    VkDevice device = m_VulkanAPI->GetDevice();

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(uint32_t);
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer staging = VK_NULL_HANDLE;
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &staging) != VK_SUCCESS) {
        return 0;
    }

    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(device, staging, &requirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = requirements.size;
    allocInfo.memoryTypeIndex = m_VulkanAPI->FindMemoryType(requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
        vkDestroyBuffer(device, staging, nullptr);
        return 0;
    }
    vkBindBufferMemory(device, staging, stagingMemory, 0);

    VkCommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandPool = m_VulkanAPI->GetCommandPool();
    cmdAllocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(device, &cmdAllocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // The pass left the attachment in its color-attachment layout; borrow it for the copy
    // and hand it back so the next frame's rendering finds what it expects.
    VulkanHelpers::TransitionImage(cmd, image->GetVkImage(),
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 1;
    region.bufferImageHeight = 1;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { (int32_t)InX, (int32_t)InY, 0 };
    region.imageExtent = { 1, 1, 1 };
    vkCmdCopyImageToBuffer(cmd, image->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, staging, 1, &region);

    VulkanHelpers::TransitionImage(cmd, image->GetVkImage(),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    vkQueueSubmit(m_VulkanAPI->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_VulkanAPI->GetGraphicsQueue());
    vkFreeCommandBuffers(device, m_VulkanAPI->GetCommandPool(), 1, &cmd);

    uint32_t value = 0;
    void* mapped = nullptr;
    if (vkMapMemory(device, stagingMemory, 0, sizeof(uint32_t), 0, &mapped) == VK_SUCCESS) {
        memcpy(&value, mapped, sizeof(uint32_t));
        vkUnmapMemory(device, stagingMemory);
    }

    vkDestroyBuffer(device, staging, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);
    return value;
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
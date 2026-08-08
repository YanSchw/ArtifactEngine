#include "VulkanFrameBuffer.h"
#include "VulkanImage.h"
#include "Helpers.h"

#include <cstring>

// Sample counts an attachment of this format can be rendered with, as a VkSampleCountFlags mask.
static VkSampleCountFlags GetSupportedSampleCounts(const VulkanAPI& InVulkanAPI, ImageFormat InFormat, VkImageUsageFlags InUsage) {
    VkImageFormatProperties properties;
    if (vkGetPhysicalDeviceImageFormatProperties(InVulkanAPI.GetPhysicalDevice(),
            VulkanHelpers::ImageFormatToVkFormat(InFormat), VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
            InUsage, 0, &properties) != VK_SUCCESS) {
        return VK_SAMPLE_COUNT_1_BIT;
    }
    return properties.sampleCounts;
}

// Halves the requested count until every attachment format supports it. SampleCount's values are
// the sample counts themselves, which is exactly what VkSampleCountFlagBits encodes.
static SampleCount ClampSampleCount(const VulkanAPI& InVulkanAPI, const FrameBufferDesc& InDesc) {
    if (!IsMultisampled(InDesc.Samples)) {
        return SampleCount::None;
    }

    VkSampleCountFlags supported = ~0u;
    for (const SharedObjectPtr<ImageView>& colorAttachment : InDesc.ColorAttachments) {
        supported &= GetSupportedSampleCounts(InVulkanAPI, colorAttachment->GetDesc().Format,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
    }
    if (InDesc.DepthAttachment) {
        supported &= GetSupportedSampleCounts(InVulkanAPI, InDesc.DepthAttachment->GetDesc().Format,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
    }

    SampleCount samples = InDesc.Samples;
    while (IsMultisampled(samples) && (supported & (VkSampleCountFlags)samples) == 0) {
        samples = (SampleCount)((uint32_t)samples / 2);
    }
    if (samples != InDesc.Samples) {
        AE_WARN("framebuffer sample count {0} is unsupported, falling back to {1}", (uint32_t)InDesc.Samples, (uint32_t)samples);
    }
    return samples;
}

// A multisampled twin of an attachment: same format and aspect, but rendered at InSamples and only
// ever alive inside the pass, so it may live in transient memory.
static SharedObjectPtr<ImageView> CreateMultisampleAttachment(const ImageView& InAttachment, ImageUsage InUsage,
                                                              uint32_t InWidth, uint32_t InHeight, SampleCount InSamples) {
    ImageDesc imageDesc;
    imageDesc.Width = InWidth;
    imageDesc.Height = InHeight;
    imageDesc.Format = InAttachment.GetDesc().Format;
    imageDesc.Usage = InUsage | ImageUsage::Transient;
    imageDesc.Samples = InSamples;

    ImageViewDesc viewDesc = InAttachment.GetDesc();
    viewDesc.ImagePtr = Image::Create(imageDesc);
    return ImageView::Create(viewDesc);
}

VulkanFrameBuffer::VulkanFrameBuffer(const FrameBufferDesc& InFrameBufferDesc, VulkanAPI& InVulkanAPI) {
    m_Desc = InFrameBufferDesc;
    m_VulkanAPI = &InVulkanAPI;
    m_Desc.Samples = ClampSampleCount(InVulkanAPI, m_Desc);

    if (IsMultisampled()) {
        CreateMultisampleAttachments();
    }

    // With dynamic rendering, we render directly to VkImageView attachments instead of creating a VkFramebuffer.
    for (int32_t i = 0; i < m_Desc.ColorAttachments.Size(); i++) {
        const SharedObjectPtr<ImageView>& colorAttachment = IsMultisampled() ? m_MultisampleColorAttachments[i] : m_Desc.ColorAttachments[i];
        m_ColorAttachmentViews.push_back(colorAttachment->As<VulkanImageView>()->GetVkImageView());
    }

    const SharedObjectPtr<ImageView>& depthAttachment = IsMultisampled() ? m_MultisampleDepthAttachment : m_Desc.DepthAttachment;
    if (depthAttachment) {
        m_DepthAttachmentView = depthAttachment->As<VulkanImageView>()->GetVkImageView();
    }
}

void VulkanFrameBuffer::CreateMultisampleAttachments() {
    for (const SharedObjectPtr<ImageView>& colorAttachment : m_Desc.ColorAttachments) {
        m_MultisampleColorAttachments.Add(CreateMultisampleAttachment(*colorAttachment, ImageUsage::ColorAttachment,
            m_Desc.Width, m_Desc.Height, m_Desc.Samples));
    }
    if (m_Desc.DepthAttachment) {
        m_MultisampleDepthAttachment = CreateMultisampleAttachment(*m_Desc.DepthAttachment, ImageUsage::DepthStencil,
            m_Desc.Width, m_Desc.Height, m_Desc.Samples);
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

        if (IsMultisampled()) {
            attachmentInfo.resolveMode = isInteger ? VK_RESOLVE_MODE_SAMPLE_ZERO_BIT : VK_RESOLVE_MODE_AVERAGE_BIT;
            attachmentInfo.resolveImageView = m_Desc.ColorAttachments[(int32_t)i]->As<VulkanImageView>()->GetVkImageView();
            attachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }

        attachmentInfos.push_back(attachmentInfo);
    }
    return attachmentInfos;
}

static void TransitionAttachment(VkCommandBuffer InCmdBuffer, const SharedObjectPtr<ImageView>& InAttachment,
                                 VkImageLayout InOldLayout, VkImageLayout InNewLayout, VkImageAspectFlags InAspect) {
    const ImageViewDesc& viewDesc = InAttachment->GetDesc();
    VulkanHelpers::TransitionImage(InCmdBuffer, viewDesc.ImagePtr->As<VulkanImage>()->GetVkImage(),
        InOldLayout, InNewLayout, InAspect, viewDesc.BaseLayer, viewDesc.LayerCount);
}

static bool IsDepthSampled(const SharedObjectPtr<ImageView>& InDepthAttachment) {
    return InDepthAttachment && (InDepthAttachment->GetDesc().ImagePtr->GetDesc().Usage & ImageUsage::Sampled) != ImageUsage::None;
}

void VulkanFrameBuffer::TransitionToAttachmentLayout(VkCommandBuffer InCmdBuffer) const {
    for (const SharedObjectPtr<ImageView>& colorAttachment : m_Desc.ColorAttachments) {
        TransitionAttachment(InCmdBuffer, colorAttachment,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    }
    for (const SharedObjectPtr<ImageView>& colorAttachment : m_MultisampleColorAttachments) {
        TransitionAttachment(InCmdBuffer, colorAttachment,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    const SharedObjectPtr<ImageView>& depthAttachment = IsMultisampled() ? m_MultisampleDepthAttachment : m_Desc.DepthAttachment;
    if (depthAttachment) {
        TransitionAttachment(InCmdBuffer, depthAttachment,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
    }
}

void VulkanFrameBuffer::TransitionToShaderReadLayout(VkCommandBuffer InCmdBuffer) const {
    for (const SharedObjectPtr<ImageView>& colorAttachment : m_Desc.ColorAttachments) {
        TransitionAttachment(InCmdBuffer, colorAttachment,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    }
    if (IsDepthSampled(m_Desc.DepthAttachment)) {
        TransitionAttachment(InCmdBuffer, m_Desc.DepthAttachment,
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
    }
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
    attachmentInfo.storeOp = IsDepthSampled(m_Desc.DepthAttachment) ? VK_ATTACHMENT_STORE_OP_STORE
                                                                    : VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentInfo.clearValue.depthStencil = {1.0f, 0};

    return attachmentInfo;
}
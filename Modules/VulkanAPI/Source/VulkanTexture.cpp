#include "VulkanTexture.h"
#include "VulkanImage.h"

#define STB_IMAGE_IMPLEMENTATION
#include "ThirdParty/stb_image.h"

static Array<VulkanTexture*> s_Textures;

VulkanTexture::VulkanTexture(const String& InFilePath, const TextureDesc& InTextureDesc, VulkanAPI& InVulkanAPI) {
    s_Textures.Add(this);
    m_VulkanAPI = &InVulkanAPI;
    
    // Load image using stb_image
    int width, height, channels;
    stbi_uc* pixels = stbi_load(InFilePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels) {
        // Fallback to 1x1 texture if loading fails
        ImageDesc imageDesc;
        imageDesc.Width = 1;
        imageDesc.Height = 1;
        imageDesc.Format = ImageFormat::RGBA8;
        imageDesc.Usage = ImageUsage::Sampled | ImageUsage::TransferDst;

        m_Image = SharedObjectPtr<Image>(new VulkanImage(imageDesc, *m_VulkanAPI));

        ImageViewDesc viewDesc;
        viewDesc.Image = m_Image;
        viewDesc.Format = ImageFormat::RGBA8;
        viewDesc.Aspect = ImageAspect::Color;

        m_DefaultView = SharedObjectPtr<ImageView>(new VulkanImageView(viewDesc, *m_VulkanAPI));
        return;
    }

    // Force RGBA format
    channels = 4;
    size_t imageSize = (size_t)width * height * channels;

    // Create Vulkan image
    ImageDesc imageDesc;
    imageDesc.Width = (uint32_t)width;
    imageDesc.Height = (uint32_t)height;
    imageDesc.Format = ImageFormat::RGBA8;
    imageDesc.Usage = ImageUsage::Sampled | ImageUsage::TransferDst;
    imageDesc.MipLevels = 1; // For now, no mipmaps

    m_Image = SharedObjectPtr<Image>(new VulkanImage(imageDesc, *m_VulkanAPI));

    // Create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = imageSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_VulkanAPI->GetDevice(), &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create staging buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_VulkanAPI->GetDevice(), stagingBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = m_VulkanAPI->FindMemoryType(memRequirements.memoryTypeBits, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(m_VulkanAPI->GetDevice(), &allocInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate staging buffer memory!");
    }

    vkBindBufferMemory(m_VulkanAPI->GetDevice(), stagingBuffer, stagingBufferMemory, 0);

    // Copy pixel data to staging buffer
    void* data;
    vkMapMemory(m_VulkanAPI->GetDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, imageSize);
    vkUnmapMemory(m_VulkanAPI->GetDevice(), stagingBufferMemory);

    // Free stb_image data
    stbi_image_free(pixels);

    // Copy from staging buffer to image
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands(*m_VulkanAPI);

    // Transition image layout to transfer destination
    AE_ASSERT(m_Image->IsA<VulkanImage>(), "Invalid image type");
    TransitionImageLayout(m_Image->As<VulkanImage>()->GetVkImage(), 
                         VK_FORMAT_R8G8B8A8_UNORM, 
                         VK_IMAGE_LAYOUT_UNDEFINED, 
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                         commandBuffer);

    // Copy buffer to image
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {(uint32_t)width, (uint32_t)height, 1};

    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, 
                          m_Image->As<VulkanImage>()->GetVkImage(), 
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Transition image layout to shader read
    TransitionImageLayout(m_Image->As<VulkanImage>()->GetVkImage(), 
                         VK_FORMAT_R8G8B8A8_UNORM, 
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
                         commandBuffer);

    EndSingleTimeCommands(commandBuffer, *m_VulkanAPI);

    // Clean up staging buffer
    vkDestroyBuffer(m_VulkanAPI->GetDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_VulkanAPI->GetDevice(), stagingBufferMemory, nullptr);

    // Create image view
    ImageViewDesc viewDesc;
    viewDesc.Image = m_Image;
    viewDesc.Format = ImageFormat::RGBA8;
    viewDesc.Aspect = ImageAspect::Color;

    m_DefaultView = SharedObjectPtr<ImageView>(new VulkanImageView(viewDesc, *m_VulkanAPI));
}

VulkanTexture::~VulkanTexture() {
    s_Textures.Remove(this);
}

SharedObjectPtr<Image> VulkanTexture::GetImage() const {
    return m_Image;
}

SharedObjectPtr<ImageView> VulkanTexture::GetDefaultView() const {
    return m_DefaultView;
}

// Helper functions for single time command buffers
VkCommandBuffer VulkanTexture::BeginSingleTimeCommands(VulkanAPI& vulkanAPI) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = vulkanAPI.GetCommandPool();
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(vulkanAPI.GetDevice(), &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VulkanTexture::EndSingleTimeCommands(VkCommandBuffer commandBuffer, VulkanAPI& vulkanAPI) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(vulkanAPI.GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vulkanAPI.GetGraphicsQueue());

    vkFreeCommandBuffers(vulkanAPI.GetDevice(), vulkanAPI.GetCommandPool(), 1, &commandBuffer);
}

void VulkanTexture::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer commandBuffer) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
}

void VulkanTexture::DestroyAll() {
    Array<VulkanTexture*> texturesToDestroy = s_Textures;
    for (VulkanTexture* texture : texturesToDestroy) {
        delete texture;
    }
    s_Textures.Clear();
}
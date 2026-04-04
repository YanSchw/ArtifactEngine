#include "VulkanBuffer.h"
#include "Core/Platform.h"

static Array<VulkanUniformBuffer*> s_UniformBuffers;
static Array<VulkanStorageBuffer*> s_StorageBuffers;

static uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VulkanAPI& InVulkanAPI) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(InVulkanAPI.GetPhysicalDevice(), &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    return 0;
}

static void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VulkanAPI& InVulkanAPI) {
    VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(InVulkanAPI.GetDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        AE_ERROR("failed to create buffer!"); exit(1);
    }

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(InVulkanAPI.GetDevice(), buffer, &memReqs);

    VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memReqs.memoryTypeBits, properties, InVulkanAPI);

    if (vkAllocateMemory(InVulkanAPI.GetDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        AE_ERROR("failed to allocate buffer memory!"); exit(1);
    }

    vkBindBufferMemory(InVulkanAPI.GetDevice(), buffer, bufferMemory, 0);
}

// UNIFORM BUFFER IMPLEMENTATION

VulkanUniformBuffer::VulkanUniformBuffer(uint32_t InBinding, VkDeviceSize InSize, VulkanAPI& InVulkanAPI) {
    s_UniformBuffers.Add(this);
    m_VulkanAPI = &InVulkanAPI;

    // Create buffer and allocate memory (omitted for brevity)
    m_Binding = InBinding;

    CreateBuffer(InSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_Buffer, m_DeviceMemory, InVulkanAPI);

    m_BufferInfo.buffer = m_Buffer;
    m_BufferInfo.offset = 0;
    m_BufferInfo.range  = InSize; // VK_WHOLE_SIZE;
}

VulkanUniformBuffer::~VulkanUniformBuffer() {
    s_UniformBuffers.Remove(this);
    
    if (m_Buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_VulkanAPI->GetDevice(), m_Buffer, nullptr);
        m_Buffer = VK_NULL_HANDLE;
    }
    if (m_DeviceMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_VulkanAPI->GetDevice(), m_DeviceMemory, nullptr);
        m_DeviceMemory = VK_NULL_HANDLE;
    }
}

void* VulkanUniformBuffer::MapData(size_t InSize, size_t InOffset) {
    void* data;
    vkMapMemory(m_VulkanAPI->GetDevice(), m_DeviceMemory, InOffset, InSize, 0, &data);
    return data;
}

void VulkanUniformBuffer::UnmapData() {
    vkUnmapMemory(m_VulkanAPI->GetDevice(), m_DeviceMemory);
}

void VulkanUniformBuffer::DestroyAll() {
    Array<VulkanUniformBuffer*> buffersToDestroy = s_UniformBuffers;
    for (VulkanUniformBuffer* buffer : buffersToDestroy) {
        delete buffer;
    }
}


// STORAGE BUFFER IMPLEMENTATION

VulkanStorageBuffer::VulkanStorageBuffer(uint32_t InBinding, VkDeviceSize InSize, VulkanAPI& InVulkanAPI) {
    s_StorageBuffers.Add(this);
    m_VulkanAPI = &InVulkanAPI;

    // Create buffer and allocate memory (omitted for brevity)
    m_Binding = InBinding;

    CreateBuffer(InSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_Buffer, m_DeviceMemory, InVulkanAPI);

    m_BufferInfo.buffer = m_Buffer;
    m_BufferInfo.offset = 0;
    m_BufferInfo.range  = InSize; // VK_WHOLE_SIZE;
}

VulkanStorageBuffer::~VulkanStorageBuffer() {
    s_StorageBuffers.Remove(this);
    
    if (m_Buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_VulkanAPI->GetDevice(), m_Buffer, nullptr);
        m_Buffer = VK_NULL_HANDLE;
    }
    if (m_DeviceMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_VulkanAPI->GetDevice(), m_DeviceMemory, nullptr);
        m_DeviceMemory = VK_NULL_HANDLE;
    }
}

void* VulkanStorageBuffer::MapData(size_t InSize, size_t InOffset) {
    void* data;
    vkMapMemory(m_VulkanAPI->GetDevice(), m_DeviceMemory, InOffset, InSize, 0, &data);
    return data;
}

void VulkanStorageBuffer::UnmapData() {
    vkUnmapMemory(m_VulkanAPI->GetDevice(), m_DeviceMemory);
}

void VulkanStorageBuffer::DestroyAll() {
    Array<VulkanStorageBuffer*> buffersToDestroy = s_StorageBuffers;
    for (VulkanStorageBuffer* buffer : buffersToDestroy) {
        delete buffer;
    }
}
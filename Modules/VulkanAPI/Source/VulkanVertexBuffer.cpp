#include "VulkanVertexBuffer.h"

extern VkBool32 GetMemoryType(uint32_t typeBits, VkFlags properties, uint32_t* typeIndex);

static Array<VulkanVertexBuffer*> s_VertexBuffers;

VulkanVertexBuffer::VulkanVertexBuffer(const Array<Vertex>& InVertices, const Array<uint32_t>& InIndices, VulkanAPI& InVulkanAPI) {
    s_VertexBuffers.Add(this);
    uint32_t verticesSize = (uint32_t)(InVertices.Size() * sizeof(InVertices[0]));
    uint32_t indicesSize = (uint32_t)(InIndices.Size() * sizeof(InIndices[0]));
    m_IndexCount = (uint32_t)InIndices.Size();

    VkMemoryAllocateInfo memAlloc = {};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkMemoryRequirements memReqs;
    void* data;

    struct StagingBuffer {
        VkDeviceMemory memory;
        VkBuffer buffer;
    };

    struct {
        StagingBuffer vertices;
        StagingBuffer indices;
    } stagingBuffers;

    // Allocate command buffer for copy operation
    VkCommandBufferAllocateInfo cmdBufInfo = {};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufInfo.commandPool = InVulkanAPI.GetCommandPool();
    cmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufInfo.commandBufferCount = 1;

    VkCommandBuffer copyCommandBuffer;
    vkAllocateCommandBuffers(InVulkanAPI.GetDevice(), &cmdBufInfo, &copyCommandBuffer);

    // First copy vertices to host accessible vertex buffer memory
    VkBufferCreateInfo vertexBufferInfo = {};
    vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertexBufferInfo.size = verticesSize;
    vertexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    vkCreateBuffer(InVulkanAPI.GetDevice(), &vertexBufferInfo, nullptr, &stagingBuffers.vertices.buffer);

    vkGetBufferMemoryRequirements(InVulkanAPI.GetDevice(), stagingBuffers.vertices.buffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
    vkAllocateMemory(InVulkanAPI.GetDevice(), &memAlloc, nullptr, &stagingBuffers.vertices.memory);

    vkMapMemory(InVulkanAPI.GetDevice(), stagingBuffers.vertices.memory, 0, verticesSize, 0, &data);
    memcpy(data, &InVertices[0], verticesSize);
    vkUnmapMemory(InVulkanAPI.GetDevice(), stagingBuffers.vertices.memory);
    vkBindBufferMemory(InVulkanAPI.GetDevice(), stagingBuffers.vertices.buffer, stagingBuffers.vertices.memory, 0);

    // Then allocate a gpu only buffer for vertices
    vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    vkCreateBuffer(InVulkanAPI.GetDevice(), &vertexBufferInfo, nullptr, &m_VertexBuffer);
    vkGetBufferMemoryRequirements(InVulkanAPI.GetDevice(), m_VertexBuffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAlloc.memoryTypeIndex);
    vkAllocateMemory(InVulkanAPI.GetDevice(), &memAlloc, nullptr, &m_VertexBufferMemory);
    vkBindBufferMemory(InVulkanAPI.GetDevice(), m_VertexBuffer, m_VertexBufferMemory, 0);

    // Next copy indices to host accessible index buffer memory
    VkBufferCreateInfo indexBufferInfo = {};
    indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    indexBufferInfo.size = indicesSize;
    indexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    vkCreateBuffer(InVulkanAPI.GetDevice(), &indexBufferInfo, nullptr, &stagingBuffers.indices.buffer);
    vkGetBufferMemoryRequirements(InVulkanAPI.GetDevice(), stagingBuffers.indices.buffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAlloc.memoryTypeIndex);
    vkAllocateMemory(InVulkanAPI.GetDevice(), &memAlloc, nullptr, &stagingBuffers.indices.memory);
    vkMapMemory(InVulkanAPI.GetDevice(), stagingBuffers.indices.memory, 0, indicesSize, 0, &data);
    memcpy(data, &InIndices[0], indicesSize);
    vkUnmapMemory(InVulkanAPI.GetDevice(), stagingBuffers.indices.memory);
    vkBindBufferMemory(InVulkanAPI.GetDevice(), stagingBuffers.indices.buffer, stagingBuffers.indices.memory, 0);

    // And allocate another gpu only buffer for indices
    indexBufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    vkCreateBuffer(InVulkanAPI.GetDevice(), &indexBufferInfo, nullptr, &m_IndexBuffer);
    vkGetBufferMemoryRequirements(InVulkanAPI.GetDevice(), m_IndexBuffer, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memAlloc.memoryTypeIndex);
    vkAllocateMemory(InVulkanAPI.GetDevice(), &memAlloc, nullptr, &m_IndexBufferMemory);
    vkBindBufferMemory(InVulkanAPI.GetDevice(), m_IndexBuffer, m_IndexBufferMemory, 0);

    // Now copy data from host visible buffer to gpu only buffer
    VkCommandBufferBeginInfo bufferBeginInfo = {};
    bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(copyCommandBuffer, &bufferBeginInfo);

    VkBufferCopy copyRegion = {};
    copyRegion.size = verticesSize;
    vkCmdCopyBuffer(copyCommandBuffer, stagingBuffers.vertices.buffer, m_VertexBuffer, 1, &copyRegion);
    copyRegion.size = indicesSize;
    vkCmdCopyBuffer(copyCommandBuffer, stagingBuffers.indices.buffer, m_IndexBuffer, 1, &copyRegion);

    vkEndCommandBuffer(copyCommandBuffer);

    // Submit to queue
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &copyCommandBuffer;

    vkQueueSubmit(InVulkanAPI.GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(InVulkanAPI.GetGraphicsQueue());

    vkFreeCommandBuffers(InVulkanAPI.GetDevice(), InVulkanAPI.GetCommandPool(), 1, &copyCommandBuffer);

    vkDestroyBuffer(InVulkanAPI.GetDevice(), stagingBuffers.vertices.buffer, nullptr);
    vkFreeMemory(InVulkanAPI.GetDevice(), stagingBuffers.vertices.memory, nullptr);
    vkDestroyBuffer(InVulkanAPI.GetDevice(), stagingBuffers.indices.buffer, nullptr);
    vkFreeMemory(InVulkanAPI.GetDevice(), stagingBuffers.indices.memory, nullptr);
}

VulkanVertexBuffer::~VulkanVertexBuffer() {
    s_VertexBuffers.Remove(this);
    VkDevice device = VulkanAPI::Get().GetDevice();
    vkDestroyBuffer(device, m_VertexBuffer, nullptr);
    vkFreeMemory(device, m_VertexBufferMemory, nullptr);
    vkDestroyBuffer(device, m_IndexBuffer, nullptr);
    vkFreeMemory(device, m_IndexBufferMemory, nullptr);
}

uint32_t VulkanVertexBuffer::GetIndexCount() const {
    return m_IndexCount;
}

void VulkanVertexBuffer::DestroyAll() {
    Array<VulkanVertexBuffer*> buffersToDestroy = s_VertexBuffers;
    for (VulkanVertexBuffer* buffer : buffersToDestroy) {
        delete buffer;
    }
    s_VertexBuffers.Clear();
}

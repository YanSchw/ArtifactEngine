#include "VulkanVertexBuffer.h"

#include <cstring>

extern VkBool32 GetMemoryType(uint32_t typeBits, VkFlags properties, uint32_t* typeIndex);

static Array<VulkanVertexBuffer*> s_VertexBuffers;

VulkanVertexBuffer::VulkanVertexBuffer(const void* InVertexData, uint32_t InVertexByteSize, const Array<uint32_t>& InIndices, VulkanAPI& InVulkanAPI) {
    s_VertexBuffers.Add(this);
    m_VulkanAPI = &InVulkanAPI;
    uint32_t verticesSize = InVertexByteSize;
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
    memcpy(data, InVertexData, verticesSize);
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

VulkanVertexBuffer::VulkanVertexBuffer(VulkanAPI& InVulkanAPI) {
    s_VertexBuffers.Add(this);
    m_VulkanAPI = &InVulkanAPI;
    m_IsDynamic = true;
    // Slots are allocated lazily on the first Update() once we know how big the frame's data is.
}

VulkanVertexBuffer::~VulkanVertexBuffer() {
    s_VertexBuffers.Remove(this);
    VkDevice device = VulkanAPI::Get().GetDevice();

    if (m_IsDynamic) {
        for (DynamicSlot& slot : m_Slots) {
            DestroySlot(slot);
        }
        return;
    }

    vkDestroyBuffer(device, m_VertexBuffer, nullptr);
    vkFreeMemory(device, m_VertexBufferMemory, nullptr);
    vkDestroyBuffer(device, m_IndexBuffer, nullptr);
    vkFreeMemory(device, m_IndexBufferMemory, nullptr);
}

void VulkanVertexBuffer::DestroySlot(DynamicSlot& InSlot) {
    VkDevice device = m_VulkanAPI->GetDevice();
    if (InSlot.VertexMapped) {
        vkUnmapMemory(device, InSlot.VertexMemory);
        InSlot.VertexMapped = nullptr;
    }
    if (InSlot.VertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, InSlot.VertexBuffer, nullptr);
        InSlot.VertexBuffer = VK_NULL_HANDLE;
    }
    if (InSlot.VertexMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, InSlot.VertexMemory, nullptr);
        InSlot.VertexMemory = VK_NULL_HANDLE;
    }
    InSlot.VertexCapacity = 0;

    if (InSlot.IndexMapped) {
        vkUnmapMemory(device, InSlot.IndexMemory);
        InSlot.IndexMapped = nullptr;
    }
    if (InSlot.IndexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, InSlot.IndexBuffer, nullptr);
        InSlot.IndexBuffer = VK_NULL_HANDLE;
    }
    if (InSlot.IndexMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, InSlot.IndexMemory, nullptr);
        InSlot.IndexMemory = VK_NULL_HANDLE;
    }
    InSlot.IndexCapacity = 0;
}

void VulkanVertexBuffer::EnsureSlotCapacity(DynamicSlot& InSlot, VkDeviceSize InVertexBytes, VkDeviceSize InIndexBytes) {
    VkDevice device = m_VulkanAPI->GetDevice();

    const VkMemoryPropertyFlags hostVisible = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    // Grow (never shrink) with 1.5x slack and a small floor.
    auto grow = [](VkDeviceSize needed) -> VkDeviceSize {
        VkDeviceSize cap = needed + needed / 2;
        return cap < 4096 ? 4096 : cap;
    };

    if (InVertexBytes > InSlot.VertexCapacity) {
        if (InSlot.VertexMapped) { vkUnmapMemory(device, InSlot.VertexMemory); InSlot.VertexMapped = nullptr; }
        if (InSlot.VertexBuffer != VK_NULL_HANDLE) { vkDestroyBuffer(device, InSlot.VertexBuffer, nullptr); }
        if (InSlot.VertexMemory != VK_NULL_HANDLE) { vkFreeMemory(device, InSlot.VertexMemory, nullptr); }

        VkDeviceSize cap = grow(InVertexBytes);
        VkBufferCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.size = cap;
        info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateBuffer(device, &info, nullptr, &InSlot.VertexBuffer);

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(device, InSlot.VertexBuffer, &memReqs);
        VkMemoryAllocateInfo memAlloc = {};
        memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memAlloc.allocationSize = memReqs.size;
        GetMemoryType(memReqs.memoryTypeBits, hostVisible, &memAlloc.memoryTypeIndex);
        vkAllocateMemory(device, &memAlloc, nullptr, &InSlot.VertexMemory);
        vkBindBufferMemory(device, InSlot.VertexBuffer, InSlot.VertexMemory, 0);
        vkMapMemory(device, InSlot.VertexMemory, 0, VK_WHOLE_SIZE, 0, &InSlot.VertexMapped);
        InSlot.VertexCapacity = cap;
    }

    if (InIndexBytes > InSlot.IndexCapacity) {
        if (InSlot.IndexMapped) { vkUnmapMemory(device, InSlot.IndexMemory); InSlot.IndexMapped = nullptr; }
        if (InSlot.IndexBuffer != VK_NULL_HANDLE) { vkDestroyBuffer(device, InSlot.IndexBuffer, nullptr); }
        if (InSlot.IndexMemory != VK_NULL_HANDLE) { vkFreeMemory(device, InSlot.IndexMemory, nullptr); }

        VkDeviceSize cap = grow(InIndexBytes);
        VkBufferCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.size = cap;
        info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateBuffer(device, &info, nullptr, &InSlot.IndexBuffer);

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(device, InSlot.IndexBuffer, &memReqs);
        VkMemoryAllocateInfo memAlloc = {};
        memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memAlloc.allocationSize = memReqs.size;
        GetMemoryType(memReqs.memoryTypeBits, hostVisible, &memAlloc.memoryTypeIndex);
        vkAllocateMemory(device, &memAlloc, nullptr, &InSlot.IndexMemory);
        vkBindBufferMemory(device, InSlot.IndexBuffer, InSlot.IndexMemory, 0);
        vkMapMemory(device, InSlot.IndexMemory, 0, VK_WHOLE_SIZE, 0, &InSlot.IndexMapped);
        InSlot.IndexCapacity = cap;
    }
}

void VulkanVertexBuffer::Update(const void* InVertexData, uint32_t InVertexByteSize, const Array<uint32_t>& InIndices) {
    AE_ASSERT(m_IsDynamic, "Update() is only valid on dynamic vertex buffers (see VertexBuffer::CreateDynamic)");

    const VkDeviceSize verticesSize = InVertexByteSize;
    const VkDeviceSize indicesSize = (VkDeviceSize)(InIndices.Size() * sizeof(uint32_t));
    m_IndexCount = (uint32_t)InIndices.Size();

    if (verticesSize == 0 || indicesSize == 0) {
        m_IndexCount = 0;
        return;
    }

    // Rotate to the next slot so we don't touch memory an in-flight frame may still be reading, then
    // upload straight into host-coherent memory.
    m_CurrentSlot = (m_CurrentSlot + 1) % kRingSize;
    DynamicSlot& slot = m_Slots[m_CurrentSlot];
    EnsureSlotCapacity(slot, verticesSize, indicesSize);

    memcpy(slot.VertexMapped, InVertexData, verticesSize);
    memcpy(slot.IndexMapped, &InIndices[0], indicesSize);

    m_VertexBuffer = slot.VertexBuffer;
    m_IndexBuffer = slot.IndexBuffer;
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

#pragma once
#include "Rendering/VertexBuffer.h"
#include "Rendering/RenderingAPI.h"
#include "VulkanVertexBuffer.gen.h"
#include "VulkanAPI.h"

#include <vulkan/vulkan.h>

class VulkanVertexBuffer : public VertexBuffer {
public:
    ARTIFACT_CLASS();

    VulkanVertexBuffer(const Array<Vertex>& InVertices, const Array<uint32_t>& InIndices, VulkanAPI& InVulkanAPI);
    virtual ~VulkanVertexBuffer();
    virtual uint32_t GetIndexCount() const override;

    static void DestroyAll();

private:
    VkBuffer m_VertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_VertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer m_IndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_IndexBufferMemory = VK_NULL_HANDLE;
    uint32_t m_IndexCount = 0;

    friend class VulkanAPI;
};
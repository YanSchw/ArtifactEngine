#pragma once
#include "Rendering/VertexBuffer.h"
#include "Rendering/RenderingAPI.h"
#include "VulkanAPI.h"

#include <vulkan/vulkan.h>
#include "VulkanVertexBuffer.gen.h"

class VulkanVertexBuffer : public VertexBuffer {
public:
    ARTIFACT_CLASS();

    /** Static buffer: stages data into device-local memory once. */
    VulkanVertexBuffer(const void* InVertexData, uint32_t InVertexByteSize, const Array<uint32_t>& InIndices, VulkanAPI& InVulkanAPI);
    /** Dynamic buffer: empty host-visible ring, re-uploaded each frame via Update(). */
    explicit VulkanVertexBuffer(VulkanAPI& InVulkanAPI);
    virtual ~VulkanVertexBuffer();
    virtual uint32_t GetIndexCount() const override;
    virtual void Update(const void* InVertexData, uint32_t InVertexByteSize, const Array<uint32_t>& InIndices) override;

    static void DestroyAll();

private:
    // A host-visible, persistently-mapped vertex+index pair. Dynamic buffers keep a small ring of
    // these and rotate each Update() so the CPU never writes memory an in-flight frame is reading.
    struct DynamicSlot {
        VkBuffer VertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory VertexMemory = VK_NULL_HANDLE;
        void* VertexMapped = nullptr;
        VkDeviceSize VertexCapacity = 0;
        VkBuffer IndexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory IndexMemory = VK_NULL_HANDLE;
        void* IndexMapped = nullptr;
        VkDeviceSize IndexCapacity = 0;
    };
    static constexpr uint32_t kRingSize = 3;

    void EnsureSlotCapacity(DynamicSlot& InSlot, VkDeviceSize InVertexBytes, VkDeviceSize InIndexBytes);
    void DestroySlot(DynamicSlot& InSlot);

    // Bound by VulkanAPI::RecordCommandBuffer. For dynamic buffers these alias the current ring slot.
    VkBuffer m_VertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_VertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer m_IndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_IndexBufferMemory = VK_NULL_HANDLE;
    uint32_t m_IndexCount = 0;

    bool m_IsDynamic = false;
    VulkanAPI* m_VulkanAPI = nullptr;
    DynamicSlot m_Slots[kRingSize];
    uint32_t m_CurrentSlot = 0;

    friend class VulkanAPI;
};
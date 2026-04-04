#pragma once
#include "Rendering/Buffer.h"
#include "Rendering/RenderingAPI.h"
#include "VulkanBuffer.gen.h"
#include "VulkanAPI.h"

#include <vulkan/vulkan.h>

class VulkanUniformBuffer : public UniformBuffer {
public:
    ARTIFACT_CLASS();

    VulkanUniformBuffer(uint32_t InBinding, VkDeviceSize InSize, VulkanAPI& InVulkanAPI);
    virtual ~VulkanUniformBuffer();
    virtual void* MapData(size_t InSize, size_t InOffset = 0) override;
    virtual void UnmapData() override;

    static void DestroyAll();

private:
    VkBuffer m_Buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_DeviceMemory = VK_NULL_HANDLE;
    VkDescriptorBufferInfo m_BufferInfo = {};
    uint32_t m_Binding = 0;
    VulkanAPI* m_VulkanAPI = nullptr;

    friend class VulkanAPI;
    friend class VulkanPipeline;
};

class VulkanStorageBuffer : public StorageBuffer {
public:
    ARTIFACT_CLASS();

    VulkanStorageBuffer(uint32_t InBinding, VkDeviceSize InSize, VulkanAPI& InVulkanAPI);
    virtual ~VulkanStorageBuffer();
    virtual void* MapData(size_t InSize, size_t InOffset = 0) override;
    virtual void UnmapData() override;

    static void DestroyAll();

private:
    VkBuffer m_Buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_DeviceMemory = VK_NULL_HANDLE;
    VkDescriptorBufferInfo m_BufferInfo = {};
    uint32_t m_Binding = 0;
    VulkanAPI* m_VulkanAPI = nullptr;

    friend class VulkanAPI;
    friend class VulkanPipeline;
};
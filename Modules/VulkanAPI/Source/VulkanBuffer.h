#pragma once
#include "Rendering/Buffer.h"
#include "Rendering/RenderingAPI.h"
#include "VulkanAPI.h"

#include <vulkan/vulkan.h>
#include "VulkanBuffer.gen.h"

class VulkanUniformBuffer : public UniformBuffer {
public:
    ARTIFACT_CLASS();

    VulkanUniformBuffer(uint32_t InBinding, VkDeviceSize InSize, VulkanAPI& InVulkanAPI);
    virtual ~VulkanUniformBuffer();
    virtual void* MapData(size_t InSize, size_t InOffset = 0) override;
    virtual void UnmapData() override;

    const VkDescriptorBufferInfo& GetDescriptorInfo() const { return m_BufferInfo; }

    static void DestroyAll();

private:
    VkBuffer m_Buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_DeviceMemory = VK_NULL_HANDLE;
    VkDescriptorBufferInfo m_BufferInfo = {};
    VulkanAPI* m_VulkanAPI = nullptr;
};

class VulkanStorageBuffer : public StorageBuffer {
public:
    ARTIFACT_CLASS();

    VulkanStorageBuffer(uint32_t InBinding, VkDeviceSize InSize, VulkanAPI& InVulkanAPI);
    virtual ~VulkanStorageBuffer();
    virtual void* MapData(size_t InSize, size_t InOffset = 0) override;
    virtual void UnmapData() override;

    const VkDescriptorBufferInfo& GetDescriptorInfo() const { return m_BufferInfo; }

    static void DestroyAll();

private:
    VkBuffer m_Buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_DeviceMemory = VK_NULL_HANDLE;
    VkDescriptorBufferInfo m_BufferInfo = {};
    VulkanAPI* m_VulkanAPI = nullptr;
};
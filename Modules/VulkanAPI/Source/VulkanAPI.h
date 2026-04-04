#pragma once
#include "Rendering/RenderingAPI.h"
#include "VulkanAPI.gen.h"

#include <vulkan/vulkan.h>

class VulkanAPI : public RenderingAPI {
public:
    ARTIFACT_CLASS();

    static VulkanAPI& Get() { return *RenderingAPI::GetInstance()->As<VulkanAPI>(); }

    virtual void Initialize() override;
    
    static void CreateInstance();
    static void CreateDebugCallback();
    static void CreateWindowSurface();
    static void FindPhysicalDevice();
    static void CheckSwapChainSupport();
    static void FindQueueFamilies();
    static void CreateLogicalDevice();
    static void CreateSemaphores();
    static void CreateCommandPool();
    static void CreateSwapChain();
    static void CreateRenderPass();
    static void CreateImageViews();
    static void CreateFramebuffers();
    static void CreateDescriptorPool();
    static void CreateCommandBuffers();
    static void UpdateCommandBuffer(size_t i);

    void RecordCommandBuffer(RenderCommandQueue& InQueue, VkCommandBuffer InCmdBuffer);

    virtual void CleanUp(bool fullClean) override;

    static void OnWindowSizeChanged();
    virtual void Draw() override;

    VkDevice GetDevice() const;
    VkPhysicalDevice GetPhysicalDevice() const;
    VkCommandPool GetCommandPool() const;
    VkDescriptorPool GetDescriptorPool() const;
    VkQueue GetGraphicsQueue() const;

    virtual class RenderCommandQueue& GetRenderQueue() override;
    virtual SharedObjectPtr<class VertexBuffer> CreateVertexBuffer(const Array<Vertex>& InVertices, const Array<uint32_t>& InIndices) override;
    virtual SharedObjectPtr<class Shader> CreateShader(const String& InShaderSource) override;
    virtual SharedObjectPtr<class Pipeline> CreatePipeline(const struct PipelineDesc& InPipelineDesc) override;
    virtual void InvalidateAllPipelines() override;
    virtual SharedObjectPtr<class UniformBuffer> CreateUniformBuffer(uint32_t InBinding, size_t InSize) override;
    virtual SharedObjectPtr<class StorageBuffer> CreateStorageBuffer(uint32_t InBinding, size_t InSize) override;

    RenderCommandQueue m_RenderQueue;
};
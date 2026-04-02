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
    static void CreateVertexDescriptions();
    static void CreateUniformBuffer();
    static void CreateSwapChain();
    static void CreateRenderPass();
    static void CreateImageViews();
    static void CreateFramebuffers();
    static void CreateGraphicsPipeline();
    static void CreateDescriptorPool();
    static void CreateDescriptorSet();
    static void CreateCommandBuffers();
    static void UpdateCommandBuffer(size_t i);

    void RecordCommandBuffer(RenderCommandQueue& InQueue, VkCommandBuffer InCmdBuffer);

    virtual void UpdateUniformData() override;

    virtual void CleanUp(bool fullClean) override;

    static void OnWindowSizeChanged();
    virtual void Draw() override;

    VkDevice GetDevice() const;
    VkCommandPool GetCommandPool() const;
    VkQueue GetGraphicsQueue() const;

    virtual class RenderCommandQueue& GetRenderQueue() override;
    virtual SharedObjectPtr<class VertexBuffer> CreateVertexBuffer(const Array<Vertex>& InVertices, const Array<uint32_t>& InIndices) override;

    RenderCommandQueue m_RenderQueue;
};
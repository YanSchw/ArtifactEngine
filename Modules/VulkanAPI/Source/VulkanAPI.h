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
    static void CreateImageViews();
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

    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    virtual class RenderCommandQueue& GetRenderQueue() override;
    virtual SharedObjectPtr<class VertexBuffer> CreateVertexBuffer(const Array<Vertex>& InVertices, const Array<uint32_t>& InIndices) override;
    virtual SharedObjectPtr<class Shader> CreateShader(const String& InShaderSource) override;
    virtual SharedObjectPtr<class Pipeline> CreatePipeline(const struct PipelineDesc& InPipelineDesc) override;
    virtual void InvalidateAllPipelines() override;
    virtual SharedObjectPtr<class UniformBuffer> CreateUniformBuffer(uint32_t InBinding, size_t InSize) override;
    virtual SharedObjectPtr<class StorageBuffer> CreateStorageBuffer(uint32_t InBinding, size_t InSize) override;
    virtual SharedObjectPtr<class Image> CreateImage(const struct ImageDesc& InImageDesc) override;
    virtual SharedObjectPtr<class ImageView> CreateImageView(const struct ImageViewDesc& InImageViewDesc) override;
    virtual SharedObjectPtr<class Texture> CreateTexture(const String& InFilePath, const TextureDesc& InTextureDesc) override;
    virtual SharedObjectPtr<class FrameBuffer> CreateFrameBuffer(const struct FrameBufferDesc& InFrameBufferDesc) override;

    RenderCommandQueue m_RenderQueue;
};
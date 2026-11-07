#pragma once
#include "Rendering/RenderingAPI.h"

#include <vulkan/vulkan.h>
#include <functional>
#include <vector>
#include "VulkanAPI.gen.h"

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
    static void CreateCommandPool();
    static void CreateDescriptorPool();
    static void CreateCommandBuffers();
    void QueryCapabilities();
    void CreateTimestampQueries();
    void ResolveTimestampQueries();

    void RecordCommandBuffer(RenderCommandQueue& InQueue, VkCommandBuffer InCmdBuffer);

    virtual void CleanUp(bool fullClean) override;
    virtual void DestroySurfaceResources(class Surface* InSurface) override;

    virtual void Draw() override;
    virtual void WaitIdle() override;

    VkDevice GetDevice() const;
    VkPhysicalDevice GetPhysicalDevice() const;
    VkCommandPool GetCommandPool() const;
    VkDescriptorPool GetDescriptorPool() const;
    VkQueue GetGraphicsQueue() const;

    /** Records InRecord into a one-off command buffer, submits it and waits for it to finish. */
    void ExecuteImmediate(const std::function<void(VkCommandBuffer)>& InRecord);

    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    bool TryFindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, uint32_t& OutMemoryTypeIndex) const;

    virtual class RenderCommandQueue& GetRenderQueue() override;
    virtual SharedObjectPtr<class VertexBuffer> CreateVertexBuffer(const void* InVertexData, uint32_t InVertexByteSize, const Array<uint32_t>& InIndices) override;
    virtual SharedObjectPtr<class VertexBuffer> CreateDynamicVertexBuffer() override;
    virtual ShaderAPI GetShaderAPI() const override { return ShaderAPI::Vulkan; }
    virtual SharedObjectPtr<class Shader> CreateShader(const CompiledShader& InCompiledShader) override;
    virtual SharedObjectPtr<class Pipeline> CreatePipeline(const struct PipelineDesc& InPipelineDesc) override;
    virtual SharedObjectPtr<class ComputePipeline> CreateComputePipeline(const struct ComputePipelineDesc& InComputePipelineDesc) override;
    virtual void InvalidateAllPipelines() override;
    virtual SharedObjectPtr<class UniformBuffer> CreateUniformBuffer(uint32_t InBinding, size_t InSize) override;
    virtual SharedObjectPtr<class StorageBuffer> CreateStorageBuffer(uint32_t InBinding, size_t InSize) override;
    virtual SharedObjectPtr<class Image> CreateImage(const struct ImageDesc& InImageDesc) override;
    virtual SharedObjectPtr<class ImageView> CreateImageView(const struct ImageViewDesc& InImageViewDesc) override;
    virtual SharedObjectPtr<class Texture> CreateTexture(byte* InPixels, uint32_t InWidth, uint32_t InHeight, uint32_t InChannels, const TextureDesc& InTextureDesc) override;
    virtual SharedObjectPtr<class Sampler> CreateSampler(const struct SamplerDesc& InSamplerDesc) override;
    virtual SharedObjectPtr<class FrameBuffer> CreateFrameBuffer(const struct FrameBufferDesc& InFrameBufferDesc) override;
    virtual SampleCount GetMaxSupportedSampleCount() const override;

    virtual uint32_t AllocateTimestampQueryPair() override;
    virtual void ReleaseTimestampQueryPair(uint32_t InQuery) override;
    virtual double GetTimestampQueryMilliseconds(uint32_t InQuery) const override;

    RenderCommandQueue m_RenderQueue;

private:
    static constexpr uint32_t MaxTimestampQueries = 4096;

    VkQueryPool m_TimestampPool = VK_NULL_HANDLE;
    double m_TimestampNanoseconds = 0.0;
    uint64_t m_TimestampMask = 0;
    bool m_TimestampsSubmitted = false;
    std::vector<bool> m_TimestampPairsInUse;
    std::vector<bool> m_TimestampsWritten;
    std::vector<double> m_TimestampPairMilliseconds;
    std::vector<uint64_t> m_TimestampResults;
};

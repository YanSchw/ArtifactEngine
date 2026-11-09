#pragma once
#include "CoreMinimal.h"
#include "Rendering/Vertex.h"
#include "Rendering/RenderingCommand.h"
#include "Rendering/Image.h"
#include "Rendering/CompiledShader.h"

#include <array>
#include "RenderingAPI.gen.h"

ARTIFACT_ENUM();
enum class RenderCapability : uint32_t {
    None           = 0,
    Compute        = 1 << 0,
    StorageBuffers = 1 << 1,
    StorageImages  = 1 << 2,
    Timestamps     = 1 << 3
};

/** What the active backend can do. */
struct RenderingCapabilities {
    bool ComputeShaders = false;
    bool StorageBuffers = false;
    bool StorageImages = false;
    bool Timestamps = false;

    std::array<uint32_t, 3> MaxComputeWorkGroupCount = { 0, 0, 0 };
    std::array<uint32_t, 3> MaxComputeWorkGroupSize = { 0, 0, 0 };
    uint32_t MaxComputeWorkGroupInvocations = 0;

    bool Supports(RenderCapability InCapabilities) const;
};

class RenderingAPI : public Object {
public:
    ARTIFACT_CLASS();

    RenderingAPI();
    static RenderingAPI* GetInstance() { return s_Instance; }

    static constexpr uint32_t InvalidTimestampQuery = ~0u;

    void DrawIndexed(uint32_t InIndexCount, uint32_t InFirstIndex, int32_t InVertexOffset);
    void SetShaderData(class ShaderData* InShaderData);
    void WriteTimestamp(uint32_t InQuery);

    const RenderingCapabilities& GetCapabilities() const { return m_Capabilities; }

    /** Reserves the queries InQuery and InQuery + 1 for timing one span of GPU work. */
    virtual uint32_t AllocateTimestampQueryPair() { return InvalidTimestampQuery; }
    virtual void ReleaseTimestampQueryPair(uint32_t InQuery) { (void)InQuery; }
    /** The span between the pair's timestamps in the latest frame the GPU has finished, 0 before one has. */
    virtual double GetTimestampQueryMilliseconds(uint32_t InQuery) const { (void)InQuery; return 0.0; }

    virtual void Initialize() = 0;
    virtual void Draw() = 0;
    virtual void CleanUp(bool InShouldDestroy) = 0;

    /** Blocks until the GPU has finished every submitted frame. Callers must do this before
     *  releasing a resource an in-flight frame still references. */
    virtual void WaitIdle() = 0;

    /** Releases everything the backend holds for the given presentation surface (its swapchain
     *  etc.). A Surface calls this before it is destroyed; unknown surfaces are a no-op. */
    virtual void DestroySurfaceResources(class Surface* InSurface) { (void)InSurface; }

    virtual class RenderCommandQueue& GetRenderQueue() = 0;
    virtual SharedObjectPtr<class VertexBuffer> CreateVertexBuffer(const void* InVertexData, uint32_t InVertexByteSize, const Array<uint32_t>& InIndices) = 0;
    virtual SharedObjectPtr<class VertexBuffer> CreateDynamicVertexBuffer() = 0;
    virtual ShaderAPI GetShaderAPI() const = 0;
    virtual SharedObjectPtr<class Shader> CreateShader(const CompiledShader& InCompiledShader) = 0;
    virtual SharedObjectPtr<class Pipeline> CreatePipeline(const struct PipelineDesc& InPipelineDesc) = 0;
    virtual SharedObjectPtr<class ComputePipeline> CreateComputePipeline(const struct ComputePipelineDesc& InComputePipelineDesc) = 0;
    virtual void InvalidateAllPipelines() = 0;
    virtual SharedObjectPtr<class UniformBuffer> CreateUniformBuffer(uint32_t InBinding, size_t InSize) = 0;
    virtual SharedObjectPtr<class StorageBuffer> CreateStorageBuffer(uint32_t InBinding, size_t InSize) = 0;
    virtual SharedObjectPtr<class Image> CreateImage(const struct ImageDesc& InImageDesc) = 0;
    virtual SharedObjectPtr<class ImageView> CreateImageView(const struct ImageViewDesc& InImageViewDesc) = 0;
    virtual SharedObjectPtr<class Texture> CreateTexture(byte* InPixels, uint32_t InWidth, uint32_t InHeight, uint32_t InChannels, const struct TextureDesc& InTextureDesc) = 0;
    virtual SharedObjectPtr<class Sampler> CreateSampler(const struct SamplerDesc& InSamplerDesc) = 0;
    virtual SharedObjectPtr<class FrameBuffer> CreateFrameBuffer(const struct FrameBufferDesc& InFrameBufferDesc) = 0;

    virtual SampleCount GetMaxSupportedSampleCount() const = 0;

protected:
    RenderingCapabilities m_Capabilities;

private:
    inline static RenderingAPI* s_Instance = nullptr;
};

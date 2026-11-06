#pragma once
#include "CoreMinimal.h"
#include "Rendering/Vertex.h"
#include "Rendering/RenderingCommand.h"
#include "Rendering/Image.h"
#include "Rendering/CompiledShader.h"

#include <array>
#include "RenderingAPI.gen.h"

/** What the active backend can do. */
struct RenderingCapabilities {
    bool ComputeShaders = false;
    bool StorageBuffers = false;
    bool StorageImages = false;

    std::array<uint32_t, 3> MaxComputeWorkGroupCount = { 0, 0, 0 };
    std::array<uint32_t, 3> MaxComputeWorkGroupSize = { 0, 0, 0 };
    uint32_t MaxComputeWorkGroupInvocations = 0;
};

class RenderingAPI : public Object {
public:
    ARTIFACT_CLASS();

    RenderingAPI();
    static RenderingAPI* GetInstance() { return s_Instance; }

    void DrawIndexed(uint32_t InIndexCount, uint32_t InFirstIndex, int32_t InVertexOffset);
    void SetShaderData(class ShaderData* InShaderData);

    const RenderingCapabilities& GetCapabilities() const { return m_Capabilities; }

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
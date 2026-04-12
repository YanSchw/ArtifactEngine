#pragma once
#include "CoreMinimal.h"
#include "Rendering/Vertex.h"
#include "Rendering/RenderingCommand.h"
#include "RenderingAPI.gen.h"

class RenderingAPI : public Object {
public:
    ARTIFACT_CLASS();

    RenderingAPI();
    static RenderingAPI* GetInstance() { return s_Instance; }

    void DrawIndexed(uint32_t InIndexCount, uint32_t InFirstIndex, int32_t InVertexOffset);

    virtual void Initialize() = 0;
    virtual void Draw() = 0;
    virtual void CleanUp(bool InShouldDestroy) = 0;

    virtual class RenderCommandQueue& GetRenderQueue() = 0;
    virtual SharedObjectPtr<class VertexBuffer> CreateVertexBuffer(const Array<Vertex>& InVertices, const Array<uint32_t>& InIndices) = 0;
    virtual SharedObjectPtr<class Shader> CreateShader(const String& InShaderSource) = 0;
    virtual SharedObjectPtr<class Pipeline> CreatePipeline(const struct PipelineDesc& InPipelineDesc) = 0;
    virtual void InvalidateAllPipelines() = 0;
    virtual SharedObjectPtr<class UniformBuffer> CreateUniformBuffer(uint32_t InBinding, size_t InSize) = 0;
    virtual SharedObjectPtr<class StorageBuffer> CreateStorageBuffer(uint32_t InBinding, size_t InSize) = 0;
    virtual SharedObjectPtr<class Image> CreateImage(const struct ImageDesc& InImageDesc) = 0;
    virtual SharedObjectPtr<class ImageView> CreateImageView(const struct ImageViewDesc& InImageViewDesc) = 0;
    virtual SharedObjectPtr<class Texture> CreateTexture(const String& InFilePath, const struct TextureDesc& InTextureDesc) = 0;
    virtual SharedObjectPtr<class Sampler> CreateSampler(const struct SamplerDesc& InSamplerDesc) = 0;
    virtual SharedObjectPtr<class FrameBuffer> CreateFrameBuffer(const struct FrameBufferDesc& InFrameBufferDesc) = 0;

private:
    inline static RenderingAPI* s_Instance = nullptr;
};
#pragma once
#include "Rendering/RenderingAPI.h"
#include "WebGLCommon.h"
#include "WebGLAPI.gen.h"

/** WebGL 2 backend. The render queue is replayed straight onto the GL context at Draw() time, so
 *  there is no command buffer to record into and no frame the driver can still be reading from. */
class WebGLAPI : public RenderingAPI {
public:
    ARTIFACT_CLASS();

    virtual void Initialize() override;
    virtual void Draw() override;
    virtual void CleanUp(bool InShouldDestroy) override;
    virtual void WaitIdle() override;
    virtual void DestroySurfaceResources(class Surface* InSurface) override;

    virtual RenderCommandQueue& GetRenderQueue() override { return m_RenderQueue; }

    virtual SharedObjectPtr<class VertexBuffer> CreateVertexBuffer(const void* InVertexData, uint32_t InVertexByteSize, const Array<uint32_t>& InIndices) override;
    virtual SharedObjectPtr<class VertexBuffer> CreateDynamicVertexBuffer() override;
    virtual ShaderAPI GetShaderAPI() const override { return ShaderAPI::WebGL; }
    virtual SharedObjectPtr<class Shader> CreateShader(const CompiledShader& InCompiledShader) override;
    virtual SharedObjectPtr<class Pipeline> CreatePipeline(const struct PipelineDesc& InPipelineDesc) override;
    virtual void InvalidateAllPipelines() override { }
    virtual SharedObjectPtr<class UniformBuffer> CreateUniformBuffer(uint32_t InBinding, size_t InSize) override;
    virtual SharedObjectPtr<class StorageBuffer> CreateStorageBuffer(uint32_t InBinding, size_t InSize) override;
    virtual SharedObjectPtr<class Image> CreateImage(const struct ImageDesc& InImageDesc) override;
    virtual SharedObjectPtr<class ImageView> CreateImageView(const struct ImageViewDesc& InImageViewDesc) override;
    virtual SharedObjectPtr<class Texture> CreateTexture(const String& InFilePath, const struct TextureDesc& InTextureDesc) override;
    virtual SharedObjectPtr<class Texture> CreateTexture(byte* InPixels, uint32_t InWidth, uint32_t InHeight, uint32_t InChannels, const struct TextureDesc& InTextureDesc) override;
    virtual SharedObjectPtr<class Sampler> CreateSampler(const struct SamplerDesc& InSamplerDesc) override;
    virtual SharedObjectPtr<class FrameBuffer> CreateFrameBuffer(const struct FrameBufferDesc& InFrameBufferDesc) override;

    virtual SampleCount GetMaxSupportedSampleCount() const override;

private:
    void Execute();
    void BeginSurfacePass(class Surface* InSurface);
    void Present();
    void DestroySurfaceFrameBuffer();

    RenderCommandQueue m_RenderQueue;
    GLuint m_ShaderDataBuffer = 0;

    // The canvas is drawn into off-screen and blitted at the end of the frame, which is where the
    // engine's top-left clip space is turned into OpenGL's bottom-left drawing buffer.
    WeakObjectPtr<class Surface> m_SurfaceOwner;
    GLuint m_SurfaceFrameBuffer = 0;
    GLuint m_SurfaceColorBuffer = 0;
    uint32_t m_SurfaceWidth = 0;
    uint32_t m_SurfaceHeight = 0;
    bool m_SurfacePassDrawn = false;
};

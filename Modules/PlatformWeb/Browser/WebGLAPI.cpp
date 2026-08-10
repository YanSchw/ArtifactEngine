#include "WebGLAPI.h"

#include "ShaderTranslator.h"
#include "WebGLBuffer.h"
#include "WebGLFrameBuffer.h"
#include "WebGLImage.h"
#include "WebGLPipeline.h"
#include "WebGLSampler.h"
#include "WebGLShader.h"
#include "WebGLTexture.h"
#include "WebGLVertexBuffer.h"

#include "Rendering/ShaderData.h"
#include "Rendering/Surface.h"

// std140 rounds the push-constant block (mat4 + uint) up to a multiple of 16.
static constexpr GLsizeiptr s_ShaderDataSize = 80;

void WebGLAPI::Initialize() {
    glGenBuffers(1, &m_ShaderDataBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, m_ShaderDataBuffer);
    glBufferData(GL_UNIFORM_BUFFER, s_ShaderDataSize, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    AE_INFO("WebGL renderer: {0}", (const char*)glGetString(GL_RENDERER));
}

void WebGLAPI::Draw() {
    m_SurfacePassDrawn = false;
    Execute();
    m_RenderQueue.Clear();
    Present();
}

void WebGLAPI::Execute() {
    WebGLFrameBuffer* openFrameBuffer = nullptr;
    WebGLPipeline* currentPipeline = nullptr;
    WebGLVertexBuffer* currentVertexBuffer = nullptr;

    for (const RenderCommand& command : m_RenderQueue.commands) {
        switch (command.Type) {
            case RenderCommandType::BeginRenderPass: {
                Object* target = std::get<CmdBeginRenderPass>(command.Data).Target.Get();
                if (openFrameBuffer) {
                    openFrameBuffer->EndPass();
                    openFrameBuffer = nullptr;
                }

                if (WebGLFrameBuffer* frameBuffer = target ? target->As<WebGLFrameBuffer>() : nullptr) {
                    frameBuffer->BeginPass();
                    openFrameBuffer = frameBuffer;
                } else if (Surface* surface = target ? target->As<Surface>() : nullptr) {
                    BeginSurfacePass(surface);
                } else {
                    AE_ERROR("Unsupported render target type");
                }
                break;
            }

            case RenderCommandType::BindPipeline: {
                Pipeline* pipeline = std::get<CmdBindPipeline>(command.Data).Pipeline.Get();
                currentPipeline = pipeline ? pipeline->As<WebGLPipeline>() : nullptr;
                if (currentPipeline) {
                    currentPipeline->Apply(m_ShaderDataBuffer);
                }
                break;
            }

            case RenderCommandType::BindVertexBuffer: {
                VertexBuffer* buffer = std::get<CmdBindVertexBuffer>(command.Data).Buffer.Get();
                currentVertexBuffer = buffer ? buffer->As<WebGLVertexBuffer>() : nullptr;
                break;
            }

            case RenderCommandType::DrawIndexed: {
                const CmdDrawIndexed& draw = std::get<CmdDrawIndexed>(command.Data);
                if (!currentVertexBuffer || !currentPipeline) {
                    break;
                }
                currentVertexBuffer->Bind(currentPipeline->GetDesc().VertexLayout);
                glDrawElements(GL_TRIANGLES, (GLsizei)draw.IndexCount, GL_UNSIGNED_INT,
                               (const void*)(size_t)(draw.FirstIndex * sizeof(uint32_t)));
                break;
            }

            case RenderCommandType::SetShaderData: {
                ShaderData* data = std::get<CmdSetShaderData>(command.Data).Data.Get();
                if (!data || data->Size() == 0) {
                    break;
                }
                glBindBuffer(GL_UNIFORM_BUFFER, m_ShaderDataBuffer);
                glBufferSubData(GL_UNIFORM_BUFFER, 0, (GLsizeiptr)data->Size(), data->Data());
                glBindBuffer(GL_UNIFORM_BUFFER, 0);
                break;
            }
        }
    }

    if (openFrameBuffer) {
        openFrameBuffer->EndPass();
    }
}

void WebGLAPI::BeginSurfacePass(Surface* InSurface) {
    const uint32_t width = glm::max(InSurface->GetWidth(), 1u);
    const uint32_t height = glm::max(InSurface->GetHeight(), 1u);

    if (m_SurfaceOwner.Get() != InSurface || width != m_SurfaceWidth || height != m_SurfaceHeight) {
        DestroySurfaceFrameBuffer();

        glGenFramebuffers(1, &m_SurfaceFrameBuffer);
        glGenRenderbuffers(1, &m_SurfaceColorBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, m_SurfaceColorBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, (GLsizei)width, (GLsizei)height);
        glBindFramebuffer(GL_FRAMEBUFFER, m_SurfaceFrameBuffer);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_SurfaceColorBuffer);

        m_SurfaceOwner = InSurface;
        m_SurfaceWidth = width;
        m_SurfaceHeight = height;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_SurfaceFrameBuffer);
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);
    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    const Vec4 clearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClearBufferfv(GL_COLOR, 0, &clearColor.x);

    m_SurfacePassDrawn = true;
}

void WebGLAPI::Present() {
    if (!m_SurfacePassDrawn) {
        return;
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_SurfaceFrameBuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, (GLint)m_SurfaceWidth, (GLint)m_SurfaceHeight,
                      0, (GLint)m_SurfaceHeight, (GLint)m_SurfaceWidth, 0,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void WebGLAPI::DestroySurfaceFrameBuffer() {
    if (m_SurfaceFrameBuffer != 0) {
        glDeleteFramebuffers(1, &m_SurfaceFrameBuffer);
        glDeleteRenderbuffers(1, &m_SurfaceColorBuffer);
        m_SurfaceFrameBuffer = 0;
        m_SurfaceColorBuffer = 0;
    }
    m_SurfaceWidth = 0;
    m_SurfaceHeight = 0;
}

void WebGLAPI::DestroySurfaceResources(Surface* InSurface) {
    if (m_SurfaceOwner.Get() == InSurface) {
        DestroySurfaceFrameBuffer();
        m_SurfaceOwner = nullptr;
    }
}

void WebGLAPI::CleanUp(bool InShouldDestroy) {
    if (!InShouldDestroy) {
        return;
    }
    DestroySurfaceFrameBuffer();
    if (m_ShaderDataBuffer != 0) {
        glDeleteBuffers(1, &m_ShaderDataBuffer);
        m_ShaderDataBuffer = 0;
    }
}

void WebGLAPI::WaitIdle() {
    glFinish();
}

SharedObjectPtr<VertexBuffer> WebGLAPI::CreateVertexBuffer(const void* InVertexData, uint32_t InVertexByteSize, const Array<uint32_t>& InIndices) {
    return new WebGLVertexBuffer(InVertexData, InVertexByteSize, InIndices, false);
}

SharedObjectPtr<VertexBuffer> WebGLAPI::CreateDynamicVertexBuffer() {
    return new WebGLVertexBuffer(nullptr, 0, {}, true);
}

SharedObjectPtr<Shader> WebGLAPI::CreateShader(const CompiledShader& InCompiledShader) {
    return new WebGLShader(InCompiledShader);
}

SharedObjectPtr<Pipeline> WebGLAPI::CreatePipeline(const PipelineDesc& InPipelineDesc) {
    return new WebGLPipeline(InPipelineDesc);
}

SharedObjectPtr<UniformBuffer> WebGLAPI::CreateUniformBuffer(uint32_t InBinding, size_t InSize) {
    return new WebGLUniformBuffer(InBinding, InSize);
}

SharedObjectPtr<StorageBuffer> WebGLAPI::CreateStorageBuffer(uint32_t InBinding, size_t InSize) {
    (void)InBinding;
    (void)InSize;
    AE_ERROR("WebGL 2 has no shader storage buffers");
    return nullptr;
}

SharedObjectPtr<Image> WebGLAPI::CreateImage(const ImageDesc& InImageDesc) {
    return new WebGLImage(InImageDesc);
}

SharedObjectPtr<ImageView> WebGLAPI::CreateImageView(const ImageViewDesc& InImageViewDesc) {
    return new WebGLImageView(InImageViewDesc);
}

SharedObjectPtr<Texture> WebGLAPI::CreateTexture(const String& InFilePath, const TextureDesc& InTextureDesc) {
    return new WebGLTexture(InFilePath, InTextureDesc);
}

SharedObjectPtr<Texture> WebGLAPI::CreateTexture(byte* InPixels, uint32_t InWidth, uint32_t InHeight, uint32_t InChannels, const TextureDesc& InTextureDesc) {
    return new WebGLTexture(InPixels, InWidth, InHeight, InChannels, InTextureDesc);
}

SharedObjectPtr<Sampler> WebGLAPI::CreateSampler(const SamplerDesc& InSamplerDesc) {
    return new WebGLSampler(InSamplerDesc);
}

SharedObjectPtr<FrameBuffer> WebGLAPI::CreateFrameBuffer(const FrameBufferDesc& InFrameBufferDesc) {
    return new WebGLFrameBuffer(InFrameBufferDesc);
}

SampleCount WebGLAPI::GetMaxSupportedSampleCount() const {
    GLint maxSamples = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);

    for (SampleCount count : { SampleCount::X8, SampleCount::X4, SampleCount::X2 }) {
        if (maxSamples >= (GLint)count) {
            return count;
        }
    }
    return SampleCount::None;
}

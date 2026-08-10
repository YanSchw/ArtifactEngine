#include "WebGLFrameBuffer.h"

#include "WebGLImage.h"

static GLsizei ResolveSampleCount(SampleCount InRequested) {
    if (!IsMultisampled(InRequested)) {
        return 0;
    }

    GLint maxSamples = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    return (GLsizei)glm::min((GLint)InRequested, maxSamples);
}

WebGLFrameBuffer::WebGLFrameBuffer(const FrameBufferDesc& InFrameBufferDesc) {
    m_Desc = InFrameBufferDesc;

    glGenFramebuffers(1, &m_FrameBuffer);
    CreateAttachments();

    if (const GLsizei samples = ResolveSampleCount(m_Desc.Samples)) {
        CreateMultisampleAttachments(samples);
    }
}

WebGLFrameBuffer::~WebGLFrameBuffer() {
    glDeleteFramebuffers(1, &m_FrameBuffer);
    if (m_MultisampleFrameBuffer != 0) {
        glDeleteFramebuffers(1, &m_MultisampleFrameBuffer);
        glDeleteRenderbuffers(m_MultisampleRenderBuffers.Size(), m_MultisampleRenderBuffers.Data());
    }
}

void WebGLFrameBuffer::CreateAttachments() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);

    for (int32_t i = 0; i < m_Desc.ColorAttachments.Size(); i++) {
        m_Desc.ColorAttachments[i]->As<WebGLImageView>()->Attach(GL_COLOR_ATTACHMENT0 + i);
    }
    if (m_Desc.DepthAttachment) {
        WebGLImageView* depth = m_Desc.DepthAttachment->As<WebGLImageView>();
        depth->Attach(GetWebGLFormat(depth->GetDesc().Format).Attachment);
    }

    SetDrawBuffers(-1);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        AE_ERROR("Incomplete frame buffer ({0}x{1})", m_Desc.Width, m_Desc.Height);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void WebGLFrameBuffer::CreateMultisampleAttachments(GLsizei InSamples) {
    glGenFramebuffers(1, &m_MultisampleFrameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_MultisampleFrameBuffer);

    const auto addRenderBuffer = [this, InSamples](ImageFormat InFormat, GLenum InAttachment) {
        GLuint renderBuffer = 0;
        glGenRenderbuffers(1, &renderBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, InSamples, GetWebGLFormat(InFormat).InternalFormat,
                                         (GLsizei)m_Desc.Width, (GLsizei)m_Desc.Height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, InAttachment, GL_RENDERBUFFER, renderBuffer);
        m_MultisampleRenderBuffers.Add(renderBuffer);
    };

    for (int32_t i = 0; i < m_Desc.ColorAttachments.Size(); i++) {
        addRenderBuffer(m_Desc.ColorAttachments[i]->GetDesc().Format, GL_COLOR_ATTACHMENT0 + i);
    }
    if (m_Desc.DepthAttachment) {
        const ImageFormat format = m_Desc.DepthAttachment->GetDesc().Format;
        addRenderBuffer(format, GetWebGLFormat(format).Attachment);
    }

    SetDrawBuffers(-1);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        AE_ERROR("Incomplete multisampled frame buffer ({0} samples)", InSamples);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void WebGLFrameBuffer::SetDrawBuffers(int32_t InSingleAttachment) {
    if (m_Desc.ColorAttachments.IsEmpty()) {
        const GLenum none = GL_NONE;
        glDrawBuffers(1, &none);
        return;
    }

    Array<GLenum> buffers;
    for (int32_t i = 0; i < m_Desc.ColorAttachments.Size(); i++) {
        const bool enabled = InSingleAttachment < 0 || InSingleAttachment == i;
        buffers.Add(enabled ? (GLenum)(GL_COLOR_ATTACHMENT0 + i) : GL_NONE);
    }
    glDrawBuffers(buffers.Size(), buffers.Data());
}

void WebGLFrameBuffer::BeginPass() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_MultisampleFrameBuffer != 0 ? m_MultisampleFrameBuffer : m_FrameBuffer);
    glViewport(0, 0, (GLsizei)m_Desc.Width, (GLsizei)m_Desc.Height);

    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);

    for (int32_t i = 0; i < m_Desc.ColorAttachments.Size(); i++) {
        const Vec4 clearColor = m_Desc.GetClearColor(i);
        if (m_Desc.ColorAttachments[i]->GetDesc().Format == ImageFormat::R32UI) {
            const GLuint value[4] = { (GLuint)clearColor.x, 0, 0, 0 };
            glClearBufferuiv(GL_COLOR, i, value);
        } else {
            glClearBufferfv(GL_COLOR, i, &clearColor.x);
        }
    }
    if (m_Desc.DepthAttachment) {
        const GLfloat depth = 1.0f;
        glClearBufferfv(GL_DEPTH, 0, &depth);
    }
}

void WebGLFrameBuffer::EndPass() {
    if (m_MultisampleFrameBuffer == 0) {
        return;
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_MultisampleFrameBuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FrameBuffer);
    for (int32_t i = 0; i < m_Desc.ColorAttachments.Size(); i++) {
        glReadBuffer(GL_COLOR_ATTACHMENT0 + i);
        SetDrawBuffers(i);
        glBlitFramebuffer(0, 0, (GLint)m_Desc.Width, (GLint)m_Desc.Height,
                          0, 0, (GLint)m_Desc.Width, (GLint)m_Desc.Height,
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBuffer);
    SetDrawBuffers(-1);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

uint32_t WebGLFrameBuffer::ReadPixelUint(int32_t InAttachment, uint32_t InX, uint32_t InY) const {
    if (InAttachment < 0 || InAttachment >= m_Desc.ColorAttachments.Size()) {
        return 0;
    }
    if (InX >= m_Desc.Width || InY >= m_Desc.Height) {
        return 0;
    }

    uint32_t value = 0;
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FrameBuffer);
    glReadBuffer(GL_COLOR_ATTACHMENT0 + InAttachment);
    glReadPixels((GLint)InX, (GLint)InY, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &value);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    return value;
}

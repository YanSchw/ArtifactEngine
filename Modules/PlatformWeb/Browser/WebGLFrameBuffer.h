#pragma once
#include "Rendering/FrameBuffer.h"
#include "WebGLCommon.h"
#include "WebGLFrameBuffer.gen.h"

class WebGLFrameBuffer : public FrameBuffer {
public:
    ARTIFACT_CLASS();

    explicit WebGLFrameBuffer(const FrameBufferDesc& InFrameBufferDesc);
    virtual ~WebGLFrameBuffer();

    /** Binds the target the pass draws into and clears it, as a Vulkan pass always would. */
    void BeginPass();
    /** Resolves the multisampled attachments back into the images the desc named. */
    void EndPass();

    virtual uint32_t ReadPixelUint(int32_t InAttachment, uint32_t InX, uint32_t InY) const override;

private:
    void CreateAttachments();
    void CreateMultisampleAttachments(GLsizei InSamples);
    void SetDrawBuffers(int32_t InSingleAttachment);

    GLuint m_FrameBuffer = 0;
    GLuint m_MultisampleFrameBuffer = 0;
    Array<GLuint> m_MultisampleRenderBuffers;
};

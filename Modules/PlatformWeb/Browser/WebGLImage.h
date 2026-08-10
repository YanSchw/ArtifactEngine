#pragma once
#include "Rendering/Image.h"
#include "WebGLCommon.h"
#include "WebGLImage.gen.h"

class WebGLImage : public Image {
public:
    ARTIFACT_CLASS();

    explicit WebGLImage(const ImageDesc& InImageDesc);
    virtual ~WebGLImage();

    GLuint GetHandle() const { return m_Texture; }
    GLenum GetTarget() const { return m_Target; }

    void Upload(const void* InPixels, uint32_t InMipLevel = 0);
    void GenerateMips();

private:
    GLuint m_Texture = 0;
    GLenum m_Target = GL_TEXTURE_2D;
};

/** WebGL 2 has no texture views, so a view is the image plus the subresource an attachment or a
 *  sampler binding should address. */
class WebGLImageView : public ImageView {
public:
    ARTIFACT_CLASS();

    explicit WebGLImageView(const ImageViewDesc& InImageViewDesc);

    WebGLImage* GetImage() const;
    GLenum GetTarget() const;

    void Bind() const;
    void Attach(GLenum InAttachment) const;
};

#include "WebGLImage.h"

WebGLImage::WebGLImage(const ImageDesc& InImageDesc) {
    m_Desc = InImageDesc;
    m_Target = InImageDesc.ArrayLayers > 1 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;

    if (IsMultisampled(InImageDesc.Samples)) {
        AE_WARN("Multisampled images are resolved inside the frame buffer on WebGL; ignoring the request");
    }

    const WebGLFormat format = GetWebGLFormat(InImageDesc.Format);

    glGenTextures(1, &m_Texture);
    glBindTexture(m_Target, m_Texture);
    if (m_Target == GL_TEXTURE_2D_ARRAY) {
        glTexStorage3D(m_Target, (GLsizei)m_Desc.MipLevels, format.InternalFormat,
                       (GLsizei)m_Desc.Width, (GLsizei)m_Desc.Height, (GLsizei)m_Desc.ArrayLayers);
    } else {
        glTexStorage2D(m_Target, (GLsizei)m_Desc.MipLevels, format.InternalFormat,
                       (GLsizei)m_Desc.Width, (GLsizei)m_Desc.Height);
    }
    glBindTexture(m_Target, 0);
}

WebGLImage::~WebGLImage() {
    if (m_Texture != 0) {
        glDeleteTextures(1, &m_Texture);
    }
}

void WebGLImage::Upload(const void* InPixels, uint32_t InMipLevel) {
    const WebGLFormat format = GetWebGLFormat(m_Desc.Format);
    const GLsizei width = (GLsizei)glm::max(m_Desc.Width >> InMipLevel, 1u);
    const GLsizei height = (GLsizei)glm::max(m_Desc.Height >> InMipLevel, 1u);

    glBindTexture(m_Target, m_Texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (m_Target == GL_TEXTURE_2D_ARRAY) {
        glTexSubImage3D(m_Target, (GLint)InMipLevel, 0, 0, 0, width, height,
                        (GLsizei)m_Desc.ArrayLayers, format.Format, format.Type, InPixels);
    } else {
        glTexSubImage2D(m_Target, (GLint)InMipLevel, 0, 0, width, height,
                        format.Format, format.Type, InPixels);
    }
    glBindTexture(m_Target, 0);
}

void WebGLImage::GenerateMips() {
    glBindTexture(m_Target, m_Texture);
    glGenerateMipmap(m_Target);
    glBindTexture(m_Target, 0);
}

WebGLImageView::WebGLImageView(const ImageViewDesc& InImageViewDesc) {
    m_Desc = InImageViewDesc;
}

WebGLImage* WebGLImageView::GetImage() const {
    return m_Desc.ImagePtr ? m_Desc.ImagePtr->As<WebGLImage>() : nullptr;
}

GLenum WebGLImageView::GetTarget() const {
    WebGLImage* image = GetImage();
    return image ? image->GetTarget() : GL_TEXTURE_2D;
}

void WebGLImageView::Bind() const {
    WebGLImage* image = GetImage();
    if (image) {
        glBindTexture(image->GetTarget(), image->GetHandle());
    }
}

void WebGLImageView::Attach(GLenum InAttachment) const {
    WebGLImage* image = GetImage();
    if (!image) {
        return;
    }

    if (image->GetTarget() == GL_TEXTURE_2D_ARRAY) {
        glFramebufferTextureLayer(GL_FRAMEBUFFER, InAttachment, image->GetHandle(),
                                  (GLint)m_Desc.BaseMip, (GLint)m_Desc.BaseLayer);
    } else {
        glFramebufferTexture2D(GL_FRAMEBUFFER, InAttachment, GL_TEXTURE_2D,
                               image->GetHandle(), (GLint)m_Desc.BaseMip);
    }
}

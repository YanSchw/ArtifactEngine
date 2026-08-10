#pragma once
#include "Rendering/Texture.h"
#include "WebGLCommon.h"
#include "WebGLTexture.gen.h"

class WebGLTexture : public Texture {
public:
    ARTIFACT_CLASS();

    WebGLTexture(const String& InFilePath, const TextureDesc& InTextureDesc);
    WebGLTexture(const byte* InPixels, uint32_t InWidth, uint32_t InHeight, uint32_t InChannels, const TextureDesc& InTextureDesc);

    virtual SharedObjectPtr<Image> GetImage() const override { return m_Image; }
    virtual SharedObjectPtr<ImageView> GetDefaultView() const override { return m_DefaultView; }

private:
    void Create(const byte* InPixels, uint32_t InWidth, uint32_t InHeight, bool InGenerateMips);

    SharedObjectPtr<Image> m_Image;
    SharedObjectPtr<ImageView> m_DefaultView;
};

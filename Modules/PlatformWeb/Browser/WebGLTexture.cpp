#include "WebGLTexture.h"

#include "WebGLImage.h"

#include <cmath>

static uint32_t GetMipCount(uint32_t InWidth, uint32_t InHeight) {
    return 1 + (uint32_t)std::floor(std::log2((float)glm::max(InWidth, InHeight)));
}

WebGLTexture::WebGLTexture(const byte* InPixels, uint32_t InWidth, uint32_t InHeight, uint32_t InChannels,
                           const TextureDesc& InTextureDesc) {
    if (!InPixels) {
        const byte magenta[4] = { 255, 0, 255, 255 };
        Create(magenta, 1, 1, ImageFormat::RGBA8, false);
        return;
    }

    if (InChannels == 4) {
        Create(InPixels, InWidth, InHeight, InTextureDesc.Format, InTextureDesc.GenerateMips);
        return;
    }

    Array<byte> rgba;
    rgba.Resize((size_t)InWidth * InHeight * 4);
    for (size_t texel = 0; texel < (size_t)InWidth * InHeight; texel++) {
        for (uint32_t channel = 0; channel < 4; channel++) {
            const bool present = channel < InChannels;
            rgba[(int32_t)(texel * 4 + channel)] = present ? InPixels[texel * InChannels + channel] : (channel == 3 ? 255 : 0);
        }
    }
    Create(rgba.Data(), InWidth, InHeight, ImageFormat::RGBA8, InTextureDesc.GenerateMips);
}

void WebGLTexture::Create(const byte* InPixels, uint32_t InWidth, uint32_t InHeight, ImageFormat InFormat, bool InGenerateMips) {
    // Generating mips needs a color-renderable format, which only RGBA8 is guaranteed to be.
    const bool mipped = InGenerateMips && InFormat == ImageFormat::RGBA8 && InWidth > 1 && InHeight > 1;

    ImageDesc imageDesc;
    imageDesc.Width = InWidth;
    imageDesc.Height = InHeight;
    imageDesc.Format = InFormat;
    imageDesc.Usage = ImageUsage::Sampled | ImageUsage::TransferDst;
    imageDesc.MipLevels = mipped ? GetMipCount(InWidth, InHeight) : 1;

    SharedObjectPtr<WebGLImage> image = new WebGLImage(imageDesc);
    image->Upload(InPixels);
    if (mipped) {
        image->GenerateMips();
    }
    m_Image = image;

    ImageViewDesc viewDesc;
    viewDesc.ImagePtr = m_Image;
    viewDesc.Format = InFormat;
    viewDesc.MipCount = imageDesc.MipLevels;
    m_DefaultView = new WebGLImageView(viewDesc);
}

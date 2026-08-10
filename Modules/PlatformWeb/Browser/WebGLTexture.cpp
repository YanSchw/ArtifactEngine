#include "WebGLTexture.h"

#include "WebGLImage.h"
#include "Serialization/ThirdParty/stb_image/stb_image.h"

#include <cmath>

static uint32_t GetMipCount(uint32_t InWidth, uint32_t InHeight) {
    return 1 + (uint32_t)std::floor(std::log2((float)glm::max(InWidth, InHeight)));
}

WebGLTexture::WebGLTexture(const String& InFilePath, const TextureDesc& InTextureDesc) {
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load(InFilePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!pixels) {
        AE_WARN("Failed to load texture '{0}'", InFilePath);
        const byte magenta[4] = { 255, 0, 255, 255 };
        Create(magenta, 1, 1, false);
        return;
    }

    Create(pixels, (uint32_t)width, (uint32_t)height, InTextureDesc.GenerateMips);
    stbi_image_free(pixels);
}

WebGLTexture::WebGLTexture(const byte* InPixels, uint32_t InWidth, uint32_t InHeight, uint32_t InChannels,
                           const TextureDesc& InTextureDesc) {
    if (!InPixels) {
        const byte magenta[4] = { 255, 0, 255, 255 };
        Create(magenta, 1, 1, false);
        return;
    }

    if (InChannels == 4) {
        Create(InPixels, InWidth, InHeight, InTextureDesc.GenerateMips);
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
    Create(rgba.Data(), InWidth, InHeight, InTextureDesc.GenerateMips);
}

void WebGLTexture::Create(const byte* InPixels, uint32_t InWidth, uint32_t InHeight, bool InGenerateMips) {
    const bool mipped = InGenerateMips && InWidth > 1 && InHeight > 1;

    ImageDesc imageDesc;
    imageDesc.Width = InWidth;
    imageDesc.Height = InHeight;
    imageDesc.Format = ImageFormat::RGBA8;
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
    viewDesc.Format = ImageFormat::RGBA8;
    viewDesc.MipCount = imageDesc.MipLevels;
    m_DefaultView = new WebGLImageView(viewDesc);
}

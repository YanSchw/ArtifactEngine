#include "Texture.h"
#include "RenderingAPI.h"

SharedObjectPtr<Texture> Texture::Create(const String& InFilePath, const TextureDesc& InTextureDesc) {
    AE_ASSERT(RenderingAPI::GetInstance(), "No rendering API instance found!");
    return RenderingAPI::GetInstance()->CreateTexture(InFilePath, InTextureDesc);
}

SharedObjectPtr<Texture> Texture::Create(byte* InPixels, uint32_t InWidth, uint32_t InHeight, uint32_t InChannels, const TextureDesc& InTextureDesc) {
    AE_ASSERT(RenderingAPI::GetInstance(), "No rendering API instance found!");
    return RenderingAPI::GetInstance()->CreateTexture(InPixels, InWidth, InHeight, InChannels, InTextureDesc);
}
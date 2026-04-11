#include "Texture.h"
#include "RenderingAPI.h"

SharedObjectPtr<Texture> Texture::Create(const String& InFilePath, const TextureDesc& InTextureDesc) {
    AE_ASSERT(RenderingAPI::GetInstance(), "No rendering API instance found!");
    return RenderingAPI::GetInstance()->CreateTexture(InFilePath, InTextureDesc);
}
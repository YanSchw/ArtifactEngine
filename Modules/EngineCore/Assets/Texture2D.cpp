#include "Texture2D.h"
#include "Rendering/Texture.h"
#include "Core/EngineConfig.h"

#define STB_IMAGE_IMPLEMENTATION
#include "Serialization/ThirdParty/stb_image/stb_image.h"

Texture2D::Texture2D() {
    m_StreamType = AssetStreamType::AlwaysLoaded;
}

void Texture2D::Load() {
#if defined(AE_PACKAGED)
    AE_ASSERT(false, "Asset loading from packaged builds is not implemented yet!");
#else
    int width, height, channels;
    String path = EngineConfig::ContentDir() + m_TexturePath;
    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    channels = 4;
    m_Texture = Texture::Create(pixels, width, height, channels);
    stbi_image_free(pixels);
#endif
}

void Texture2D::Unload() {
    m_Texture = nullptr;
}

bool Texture2D::IsLoaded() const {
    return Super::IsLoaded() && GetTexture() != nullptr;
}

Texture* Texture2D::GetTexture() const {
    return m_Texture;
}

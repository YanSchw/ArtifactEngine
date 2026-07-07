#include "Texture2D.h"
#include "Rendering/Texture.h"
#include "Core/EngineConfig.h"
#include "Serialization/ChunkedBinary.h"

#define STB_IMAGE_IMPLEMENTATION
#include "Serialization/ThirdParty/stb_image/stb_image.h"

Texture2D::Texture2D() {
    m_StreamType = AssetStreamType::AlwaysLoaded;
}

void Texture2D::Load() {
#if defined(AE_PACKAGED)
    ChunkReader chunkReader = GetChunkedBinary()->GetChunk(1);
    int width, height, channels;
    chunkReader >> width;
    chunkReader >> height;
    chunkReader >> channels;
    byte* pixels = new byte[width * height * channels];
    chunkReader.ReadBytes(pixels, width * height * channels);
    m_Texture = Texture::Create(pixels, width, height, channels);
    delete[] pixels;
#else
    int width, height, channels;
    String path = EngineConfig::ResolveContentPath(m_TexturePath);
    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    channels = 4;
    m_Texture = Texture::Create(pixels, width, height, channels);
    stbi_image_free(pixels);
#endif
}

void Texture2D::Unload() {
    m_Texture = nullptr;
}

void Texture2D::Cook(ChunkedBinary& OutChunkedBinary) {
    Super::Cook(OutChunkedBinary);

    int width, height, channels;
    String path = EngineConfig::ResolveContentPath(m_TexturePath);
    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    channels = 4;

    ChunkWriter chunkWriter;
    chunkWriter << width;
    chunkWriter << height;
    chunkWriter << channels;
    chunkWriter.WriteBytes(pixels, width * height * channels);
    OutChunkedBinary.AddChunk(1, chunkWriter);

    stbi_image_free(pixels);
}

bool Texture2D::IsLoaded() const {
    return Super::IsLoaded() && GetTexture() != nullptr;
}

Texture* Texture2D::GetTexture() const {
    return m_Texture;
}

#include "Texture2D.h"
#include "Rendering/Texture.h"
#include "Core/EngineConfig.h"
#include "Serialization/ChunkedBinary.h"
#include "Serialization/Assets/Texture2DImporter.h"

Texture2D::Texture2D() {
    m_StreamType = AssetStreamType::AlwaysLoaded;
}

bool Texture2D::ImportSource(Array<byte>& OutPixels, uint32_t& OutWidth, uint32_t& OutHeight) const {
    const String path = EngineConfig::ResolveContentPath(m_TexturePath);

    Texture2DImporter* importer = AssetImporter::Find<Texture2DImporter>(path);
    if (!importer || !importer->Import(path, OutPixels, OutWidth, OutHeight)) {
        AE_WARN("Importing texture from file {} was unsuccessful!", path);
        return false;
    }
    return true;
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
    Array<byte> pixels;
    uint32_t width = 0;
    uint32_t height = 0;
    m_Texture = ImportSource(pixels, width, height)
        ? Texture::Create(pixels.Data(), width, height, 4)
        : Texture::Create(nullptr, 1, 1, 4);
#endif
}

void Texture2D::Unload() {
    m_Texture = nullptr;
}

void Texture2D::Cook(ChunkedBinary& OutChunkedBinary) {
    Super::Cook(OutChunkedBinary);

    Array<byte> pixels;
    uint32_t width = 0;
    uint32_t height = 0;
    if (!ImportSource(pixels, width, height)) {
        return;
    }

    ChunkWriter chunkWriter;
    chunkWriter << (int)width;
    chunkWriter << (int)height;
    chunkWriter << 4;
    chunkWriter.WriteBytes(pixels.Data(), pixels.Size());
    OutChunkedBinary.AddChunk(1, chunkWriter);
}

bool Texture2D::IsLoaded() const {
    return Super::IsLoaded() && GetTexture() != nullptr;
}

Texture* Texture2D::GetTexture() const {
    return m_Texture;
}

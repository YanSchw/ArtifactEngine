#include "Texture2D.h"
#include "Rendering/Texture.h"
#include "Core/EngineConfig.h"
#include "Serialization/ChunkedBinary.h"
#include "Serialization/Assets/Texture2DImporter.h"

#include <algorithm>
#include <glm/gtc/packing.hpp>

Texture2D::Texture2D() {
    m_StreamType = AssetStreamType::AlwaysLoaded;
}

static void PackHalfFloats(const Array<float>& InPixels, Array<byte>& OutPixels) {
    OutPixels.Resize(InPixels.Size() * sizeof(uint16_t));
    uint16_t* halves = (uint16_t*)OutPixels.Data();
    for (int32_t index = 0; index < InPixels.Size(); index++) {
        halves[index] = glm::packHalf1x16(glm::min(InPixels[index], 65504.0f));
    }
}

bool Texture2D::ImportSource(Array<byte>& OutPixels, uint32_t& OutWidth, uint32_t& OutHeight, ImageFormat& OutFormat) const {
    const String path = EngineConfig::ResolveContentPath(m_TexturePath);

    Texture2DImporter* importer = AssetImporter::Find<Texture2DImporter>(path);
    bool imported = false;
    if (importer && importer->IsHighDynamicRange(path)) {
        Array<float> pixels;
        imported = importer->ImportHighDynamicRange(path, pixels, OutWidth, OutHeight);
        PackHalfFloats(pixels, OutPixels);
        OutFormat = ImageFormat::RGBA16F;
    } else if (importer) {
        imported = importer->Import(path, OutPixels, OutWidth, OutHeight);
        OutFormat = ImageFormat::RGBA8;
    }
    if (!imported) {
        AE_WARN("Importing texture from file {} was unsuccessful!", path);
        return false;
    }

    if (m_FlipVertically) {
        const size_t rowSize = (size_t)OutWidth * GetBytesPerPixel(OutFormat);
        for (uint32_t y = 0; y < OutHeight / 2; y++) {
            std::swap_ranges(OutPixels.Data() + y * rowSize, OutPixels.Data() + (y + 1) * rowSize,
                             OutPixels.Data() + (OutHeight - 1 - y) * rowSize);
        }
    }
    return true;
}

void Texture2D::Load() {
    TextureDesc desc;
    desc.GenerateMips = m_GenerateMips;
#if defined(AE_PACKAGED)
    ChunkReader chunkReader = GetChunkedBinary()->GetChunk(1);
    int width, height, channels, format;
    chunkReader >> width;
    chunkReader >> height;
    chunkReader >> channels;
    chunkReader >> format;
    desc.Format = (ImageFormat)format;
    Array<byte> pixels;
    pixels.Resize((size_t)width * height * GetBytesPerPixel(desc.Format));
    chunkReader.ReadBytes(pixels.Data(), pixels.Size());
    m_Texture = Texture::Create(pixels.Data(), width, height, channels, desc);
#else
    Array<byte> pixels;
    uint32_t width = 0;
    uint32_t height = 0;
    m_Texture = ImportSource(pixels, width, height, desc.Format)
        ? Texture::Create(pixels.Data(), width, height, 4, desc)
        : Texture::Create(nullptr, 1, 1, 4, desc);
#endif
}

void Texture2D::Reimport() {
    if (IsLoaded()) {
        Load();
    }
}

void Texture2D::Unload() {
    m_Texture = nullptr;
}

void Texture2D::Cook(ChunkedBinary& OutChunkedBinary) {
    Super::Cook(OutChunkedBinary);

    Array<byte> pixels;
    uint32_t width = 0;
    uint32_t height = 0;
    ImageFormat format = ImageFormat::RGBA8;
    if (!ImportSource(pixels, width, height, format)) {
        return;
    }

    ChunkWriter chunkWriter;
    chunkWriter << (int)width;
    chunkWriter << (int)height;
    chunkWriter << 4;
    chunkWriter << (int)format;
    chunkWriter.WriteBytes(pixels.Data(), pixels.Size());
    OutChunkedBinary.AddChunk(1, chunkWriter);
}

bool Texture2D::IsLoaded() const {
    return Super::IsLoaded() && GetTexture() != nullptr;
}

Texture* Texture2D::GetTexture() const {
    return m_Texture;
}

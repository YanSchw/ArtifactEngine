#pragma once
#include "Common/Types.h"
#include "Object/Enum.h"
#include "AssetCompression.gen.h"

ARTIFACT_ENUM();
enum class MeshCompression : uint8_t {
    UseProjectDefault = 0,
    Uncompressed = 1,
    Quantized = 2
};

ARTIFACT_ENUM();
enum class TextureCompression : uint8_t {
    UseProjectDefault = 0,
    Uncompressed = 1,
    QOI = 2
};

ARTIFACT_ENUM();
enum class AudioCompression : uint8_t {
    UseProjectDefault = 0,
    Uncompressed = 1,
    ADPCM = 2
};

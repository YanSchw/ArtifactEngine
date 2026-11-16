#pragma once
#include "Common/Types.h"

class ChunkReader;
class ChunkWriter;

class QoiCodec {
public:
    static void Encode(const byte* InPixels, size_t InPixelCount, ChunkWriter& OutWriter);
    static void Decode(ChunkReader& InReader, byte* OutPixels, size_t InPixelCount);
};

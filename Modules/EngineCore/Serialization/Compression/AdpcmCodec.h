#pragma once
#include "Common/Types.h"

class ChunkReader;
class ChunkWriter;

class AdpcmCodec {
public:
    static void Encode(const int16_t* InSamples, size_t InSampleCount, uint32_t InChannels, ChunkWriter& OutWriter);
    static void Decode(ChunkReader& InReader, int16_t* OutSamples, size_t InSampleCount, uint32_t InChannels);
};

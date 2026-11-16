#include "AdpcmCodec.h"
#include "Serialization/ChunkedBinary.h"

#include <algorithm>
#include <vector>

namespace {

constexpr int32_t g_StepTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

constexpr int32_t g_IndexTable[16] = { -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8 };

struct ChannelState {
    int32_t Predictor = 0;
    int32_t StepIndex = 0;

    uint8_t Quantize(int16_t InSample) const {
        const int32_t step = g_StepTable[StepIndex];
        int32_t difference = InSample - Predictor;
        uint8_t nibble = 0;
        if (difference < 0) {
            nibble = 8;
            difference = -difference;
        }
        if (difference >= step) {
            nibble |= 4;
            difference -= step;
        }
        if (difference >= step >> 1) {
            nibble |= 2;
            difference -= step >> 1;
        }
        if (difference >= step >> 2) {
            nibble |= 1;
        }
        return nibble;
    }

    int16_t Apply(uint8_t InNibble) {
        const int32_t step = g_StepTable[StepIndex];
        int32_t delta = step >> 3;
        if (InNibble & 4) {
            delta += step;
        }
        if (InNibble & 2) {
            delta += step >> 1;
        }
        if (InNibble & 1) {
            delta += step >> 2;
        }
        Predictor = std::clamp(Predictor + (InNibble & 8 ? -delta : delta), -32768, 32767);
        StepIndex = std::clamp(StepIndex + g_IndexTable[InNibble], 0, 88);
        return (int16_t)Predictor;
    }
};

} // namespace

void AdpcmCodec::Encode(const int16_t* InSamples, size_t InSampleCount, uint32_t InChannels, ChunkWriter& OutWriter) {
    std::vector<byte> encoded((InSampleCount + 1) / 2, 0);
    std::vector<ChannelState> channels(InChannels);
    for (size_t i = 0; i < InSampleCount; i++) {
        ChannelState& channel = channels[i % InChannels];
        const uint8_t nibble = channel.Quantize(InSamples[i]);
        channel.Apply(nibble);
        encoded[i / 2] |= i % 2 == 0 ? nibble : (uint8_t)(nibble << 4);
    }
    OutWriter.WriteBytes(encoded.data(), encoded.size());
}

void AdpcmCodec::Decode(ChunkReader& InReader, int16_t* OutSamples, size_t InSampleCount, uint32_t InChannels) {
    const Array<byte> encoded = InReader.ReadRemaining();
    std::vector<ChannelState> channels(InChannels);
    const size_t sampleCount = std::min(InSampleCount, (size_t)encoded.Size() * 2);
    for (size_t i = 0; i < sampleCount; i++) {
        const byte packed = encoded.Data()[i / 2];
        OutSamples[i] = channels[i % InChannels].Apply(i % 2 == 0 ? packed & 0x0f : packed >> 4);
    }
}

#include "QoiCodec.h"
#include "Serialization/ChunkedBinary.h"

#include <cstring>

namespace {

struct Pixel {
    uint8_t R = 0;
    uint8_t G = 0;
    uint8_t B = 0;
    uint8_t A = 0;

    bool operator==(const Pixel& InOther) const = default;
};

constexpr Pixel g_StartPixel = { 0, 0, 0, 255 };

constexpr uint8_t g_OpIndex = 0x00;
constexpr uint8_t g_OpDiff = 0x40;
constexpr uint8_t g_OpLuma = 0x80;
constexpr uint8_t g_OpRun = 0xc0;
constexpr uint8_t g_OpRgb = 0xfe;
constexpr uint8_t g_OpRgba = 0xff;
constexpr uint8_t g_OpMask = 0xc0;
constexpr int32_t g_MaxRun = 62;
constexpr size_t g_MaxOpSize = 5;

int32_t Hash(const Pixel& InPixel) {
    return (InPixel.R * 3 + InPixel.G * 5 + InPixel.B * 7 + InPixel.A * 11) % 64;
}

bool InRange(int32_t InValue, int32_t InMin, int32_t InMax) {
    return InValue >= InMin && InValue <= InMax;
}

} // namespace

void QoiCodec::Encode(const byte* InPixels, size_t InPixelCount, ChunkWriter& OutWriter) {
    Array<byte> encoded;
    Pixel seen[64] = {};
    Pixel previous = g_StartPixel;
    int32_t run = 0;

    for (size_t i = 0; i < InPixelCount; i++) {
        Pixel pixel;
        std::memcpy(&pixel, InPixels + i * 4, 4);

        if (pixel == previous) {
            if (++run == g_MaxRun || i + 1 == InPixelCount) {
                encoded.Add((byte)(g_OpRun | (run - 1)));
                run = 0;
            }
            continue;
        }
        if (run > 0) {
            encoded.Add((byte)(g_OpRun | (run - 1)));
            run = 0;
        }

        const int32_t hash = Hash(pixel);
        if (seen[hash] == pixel) {
            encoded.Add((byte)(g_OpIndex | hash));
        } else if (pixel.A != previous.A) {
            seen[hash] = pixel;
            encoded.Add(g_OpRgba);
            encoded.Add(pixel.R);
            encoded.Add(pixel.G);
            encoded.Add(pixel.B);
            encoded.Add(pixel.A);
        } else {
            seen[hash] = pixel;
            const int8_t dr = (int8_t)(pixel.R - previous.R);
            const int8_t dg = (int8_t)(pixel.G - previous.G);
            const int8_t db = (int8_t)(pixel.B - previous.B);
            const int8_t drg = (int8_t)(dr - dg);
            const int8_t dbg = (int8_t)(db - dg);

            if (InRange(dr, -2, 1) && InRange(dg, -2, 1) && InRange(db, -2, 1)) {
                encoded.Add((byte)(g_OpDiff | (dr + 2) << 4 | (dg + 2) << 2 | (db + 2)));
            } else if (InRange(drg, -8, 7) && InRange(dg, -32, 31) && InRange(dbg, -8, 7)) {
                encoded.Add((byte)(g_OpLuma | (dg + 32)));
                encoded.Add((byte)((drg + 8) << 4 | (dbg + 8)));
            } else {
                encoded.Add(g_OpRgb);
                encoded.Add(pixel.R);
                encoded.Add(pixel.G);
                encoded.Add(pixel.B);
            }
        }
        previous = pixel;
    }

    for (size_t i = 1; i < g_MaxOpSize; i++) {
        encoded.Add(0);
    }
    OutWriter.WriteBytes(encoded.Data(), encoded.Size());
}

void QoiCodec::Decode(ChunkReader& InReader, byte* OutPixels, size_t InPixelCount) {
    const Array<byte> encoded = InReader.ReadRemaining();
    const byte* data = encoded.Data();
    const size_t size = encoded.Size();

    Pixel seen[64] = {};
    Pixel pixel = g_StartPixel;
    size_t cursor = 0;
    int32_t run = 0;

    for (size_t i = 0; i < InPixelCount; i++) {
        if (run > 0) {
            run--;
        } else if (cursor + g_MaxOpSize <= size) {
            const uint8_t op = data[cursor++];
            if (op == g_OpRgb) {
                pixel.R = data[cursor++];
                pixel.G = data[cursor++];
                pixel.B = data[cursor++];
            } else if (op == g_OpRgba) {
                pixel.R = data[cursor++];
                pixel.G = data[cursor++];
                pixel.B = data[cursor++];
                pixel.A = data[cursor++];
            } else if ((op & g_OpMask) == g_OpIndex) {
                pixel = seen[op];
            } else if ((op & g_OpMask) == g_OpDiff) {
                pixel.R = (uint8_t)(pixel.R + ((op >> 4) & 0x03) - 2);
                pixel.G = (uint8_t)(pixel.G + ((op >> 2) & 0x03) - 2);
                pixel.B = (uint8_t)(pixel.B + (op & 0x03) - 2);
            } else if ((op & g_OpMask) == g_OpLuma) {
                const int32_t dg = (op & 0x3f) - 32;
                const uint8_t next = data[cursor++];
                pixel.R = (uint8_t)(pixel.R + dg - 8 + (next >> 4));
                pixel.G = (uint8_t)(pixel.G + dg);
                pixel.B = (uint8_t)(pixel.B + dg - 8 + (next & 0x0f));
            } else {
                run = op & 0x3f;
            }
            seen[Hash(pixel)] = pixel;
        }
        std::memcpy(OutPixels + i * 4, &pixel, 4);
    }
}

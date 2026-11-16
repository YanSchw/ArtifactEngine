#include "MeshQuantizer.h"
#include "Serialization/ChunkedBinary.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace {

constexpr int32_t g_ComponentCount = sizeof(Vertex) / sizeof(float);
static_assert(sizeof(Vertex) == g_ComponentCount * sizeof(float));

constexpr float g_QuantizedMax = 65535.0f;

float GetComponent(const Vertex& InVertex, int32_t InComponent) {
    float value;
    std::memcpy(&value, (const byte*)&InVertex + InComponent * sizeof(float), sizeof(float));
    return value;
}

void SetComponent(Vertex& OutVertex, int32_t InComponent, float InValue) {
    std::memcpy((byte*)&OutVertex + InComponent * sizeof(float), &InValue, sizeof(float));
}

uint32_t ZigZag(int32_t InValue) {
    return ((uint32_t)InValue << 1) ^ (uint32_t)(InValue >> 31);
}

int32_t UnZigZag(uint32_t InValue) {
    return (int32_t)(InValue >> 1) ^ -(int32_t)(InValue & 1);
}

void WriteVarUInt(Array<byte>& OutBytes, uint32_t InValue) {
    while (InValue >= 0x80) {
        OutBytes.Add((byte)(InValue | 0x80));
        InValue >>= 7;
    }
    OutBytes.Add((byte)InValue);
}

class VarUIntReader {
public:
    explicit VarUIntReader(ChunkReader& InReader) : m_Bytes(InReader.ReadRemaining()) {}

    uint32_t Next() {
        uint32_t value = 0;
        for (int32_t shift = 0; shift < 32 && m_Cursor < (size_t)m_Bytes.Size(); shift += 7) {
            const byte next = m_Bytes.Data()[m_Cursor++];
            value |= (uint32_t)(next & 0x7f) << shift;
            if (!(next & 0x80)) {
                break;
            }
        }
        return value;
    }

private:
    Array<byte> m_Bytes;
    size_t m_Cursor = 0;
};

} // namespace

void MeshQuantizer::EncodeVertices(const Array<Vertex>& InVertices, ChunkWriter& OutWriter) {
    float minimum[g_ComponentCount] = {};
    float maximum[g_ComponentCount] = {};
    for (int32_t component = 0; component < g_ComponentCount; component++) {
        for (int32_t i = 0; i < InVertices.Size(); i++) {
            const float value = GetComponent(InVertices[i], component);
            minimum[component] = i == 0 ? value : std::min(minimum[component], value);
            maximum[component] = i == 0 ? value : std::max(maximum[component], value);
        }
        OutWriter << minimum[component] << maximum[component];
    }

    Array<byte> encoded;
    for (int32_t component = 0; component < g_ComponentCount; component++) {
        if (maximum[component] <= minimum[component]) {
            continue;
        }
        const float scale = g_QuantizedMax / (maximum[component] - minimum[component]);
        int32_t previous = 0;
        for (const Vertex& vertex : InVertices) {
            const int32_t quantized = (int32_t)std::lround((GetComponent(vertex, component) - minimum[component]) * scale);
            WriteVarUInt(encoded, ZigZag(quantized - previous));
            previous = quantized;
        }
    }
    OutWriter.WriteBytes(encoded.Data(), encoded.Size());
}

void MeshQuantizer::DecodeVertices(ChunkReader& InReader, Array<Vertex>& OutVertices) {
    float minimum[g_ComponentCount];
    float maximum[g_ComponentCount];
    for (int32_t component = 0; component < g_ComponentCount; component++) {
        InReader >> minimum[component] >> maximum[component];
    }

    VarUIntReader reader(InReader);
    for (int32_t component = 0; component < g_ComponentCount; component++) {
        const bool varies = maximum[component] > minimum[component];
        const float step = varies ? (maximum[component] - minimum[component]) / g_QuantizedMax : 0.0f;
        int32_t quantized = 0;
        for (Vertex& vertex : OutVertices) {
            quantized += varies ? UnZigZag(reader.Next()) : 0;
            SetComponent(vertex, component, minimum[component] + (float)quantized * step);
        }
    }

    for (Vertex& vertex : OutVertices) {
        vertex.Normal = glm::dot(vertex.Normal, vertex.Normal) > 0.0f ? glm::normalize(vertex.Normal) : VecUtils::Up;
    }
}

void MeshQuantizer::EncodeIndices(const Array<uint32_t>& InIndices, ChunkWriter& OutWriter) {
    Array<byte> encoded;
    uint32_t previous = 0;
    for (uint32_t index : InIndices) {
        WriteVarUInt(encoded, ZigZag((int32_t)(index - previous)));
        previous = index;
    }
    OutWriter.WriteBytes(encoded.Data(), encoded.Size());
}

void MeshQuantizer::DecodeIndices(ChunkReader& InReader, Array<uint32_t>& OutIndices) {
    VarUIntReader reader(InReader);
    uint32_t previous = 0;
    for (uint32_t& index : OutIndices) {
        index = previous + (uint32_t)UnZigZag(reader.Next());
        previous = index;
    }
}

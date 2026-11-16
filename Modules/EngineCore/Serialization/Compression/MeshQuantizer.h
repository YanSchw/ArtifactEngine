#pragma once
#include "Common/Types.h"
#include "Rendering/Vertex.h"

class ChunkReader;
class ChunkWriter;

class MeshQuantizer {
public:
    static void EncodeVertices(const Array<Vertex>& InVertices, ChunkWriter& OutWriter);
    static void DecodeVertices(ChunkReader& InReader, Array<Vertex>& OutVertices);

    static void EncodeIndices(const Array<uint32_t>& InIndices, ChunkWriter& OutWriter);
    static void DecodeIndices(ChunkReader& InReader, Array<uint32_t>& OutIndices);
};

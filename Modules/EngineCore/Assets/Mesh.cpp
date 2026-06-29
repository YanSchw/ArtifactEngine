#include "Mesh.h"
#include "Rendering/Vertex.h"
#include "Rendering/VertexBuffer.h"
#include "Core/EngineConfig.h"
#include "Serialization/ChunkedBinary.h"
#include "Serialization/Assets/MeshLoader.h"

Mesh::Mesh() {
    m_StreamType = AssetStreamType::AlwaysLoaded;
}

void Mesh::Load() {
#if defined(AE_PACKAGED)
    int32_t vertexSize, indexSize;
    {
        ChunkReader chunkReader = GetChunkedBinary()->GetChunk(1);
        chunkReader >> vertexSize;
        chunkReader >> indexSize;
    }
    Array<Vertex> vertices(vertexSize);
    Array<uint32_t> indices(indexSize);
    {
        ChunkReader chunkReader = GetChunkedBinary()->GetChunk(2);
        chunkReader.ReadBytes(&vertices[0], sizeof(Vertex) * vertexSize);
    }
    {
        ChunkReader chunkReader = GetChunkedBinary()->GetChunk(3);
        chunkReader.ReadBytes(&indices[0], sizeof(uint32_t) * indexSize);
    }

    m_VertexBuffer = VertexBuffer::Create(vertices, indices);
#else
    Array<Vertex> vertices;
    Array<uint32_t> indices;
    String path = EngineConfig::ContentDir() + m_MeshPath;

    Array<Class> meshLoaderClasses = Class::GetSubclassesOf(MeshLoader::StaticClass());
    AE_ASSERT(meshLoaderClasses.Size() > 1, "No MeshLoader classes found!");
    SharedObjectPtr<MeshLoader> meshLoader = Object::Create<MeshLoader>(meshLoaderClasses[1]);
    bool success = meshLoader->LoadMeshFromFile(path, vertices, indices);
    if (!success) {
        AE_WARN("Loading mesh from file {} was unsuccessful!", path);
    }

    m_VertexBuffer = VertexBuffer::Create(vertices, indices);
#endif
}

void Mesh::Unload() {
    m_VertexBuffer = nullptr;
}

void Mesh::Cook(ChunkedBinary& OutChunkedBinary) {
    Super::Cook(OutChunkedBinary);

    Array<Vertex> vertices;
    Array<uint32_t> indices;
    String path = EngineConfig::ContentDir() + m_MeshPath;

    Array<Class> meshLoaderClasses = Class::GetSubclassesOf(MeshLoader::StaticClass());
    AE_ASSERT(meshLoaderClasses.Size() > 1, "No MeshLoader classes found!");
    SharedObjectPtr<MeshLoader> meshLoader = Object::Create<MeshLoader>(meshLoaderClasses[1]);
    meshLoader->LoadMeshFromFile(path, vertices, indices);

    {
        ChunkWriter chunkWriter;
        chunkWriter << (int32_t)vertices.Size();
        chunkWriter << (int32_t)indices.Size();
        OutChunkedBinary.AddChunk(1, chunkWriter);
    }
    {
        ChunkWriter chunkWriter;
        chunkWriter.WriteBytes(&vertices[0], sizeof(Vertex) * vertices.Size());
        OutChunkedBinary.AddChunk(2, chunkWriter);
    }
    {
        ChunkWriter chunkWriter;
        chunkWriter.WriteBytes(&indices[0], sizeof(uint32_t) * indices.Size());
        OutChunkedBinary.AddChunk(3, chunkWriter);
    }
}

bool Mesh::IsLoaded() const {
    return Super::IsLoaded() && GetVertexBuffer() != nullptr;
}

VertexBuffer* Mesh::GetVertexBuffer() const {
    return m_VertexBuffer;
}

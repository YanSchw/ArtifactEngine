#include "Mesh.h"
#include "Assets/AssetManager.h"
#include "Assets/Material.h"
#include "Rendering/Vertex.h"
#include "Rendering/VertexBuffer.h"
#include "Core/EngineConfig.h"
#include "Serialization/ChunkedBinary.h"
#include "Serialization/Assets/MeshLoader.h"

Mesh::Mesh() {
    m_StreamType = AssetStreamType::AlwaysLoaded;
}

bool Mesh::ImportSource(Array<Vertex>& OutVertices, Array<uint32_t>& OutIndices) const {
    const String path = EngineConfig::ResolveContentPath(m_MeshPath);

    Array<Class> meshLoaderClasses = Class::GetSubclassesOf(MeshLoader::StaticClass());
    AE_ASSERT(meshLoaderClasses.Size() > 1, "No MeshLoader classes found!");
    SharedObjectPtr<MeshLoader> meshLoader = Object::Create<MeshLoader>(meshLoaderClasses[1]);
    if (!meshLoader->LoadMeshFromFile(path, OutVertices, OutIndices)) {
        AE_WARN("Loading mesh from file {} was unsuccessful!", path);
        return false;
    }

    // A non-uniform import scale skews the surface, so normals follow the inverse scale.
    const Vec3 normalScale = 1.0f / glm::max(glm::abs(m_ImportScale), Vec3(1e-4f));
    for (Vertex& vertex : OutVertices) {
        vertex.Position = vertex.Position * m_ImportScale + m_ImportOffset;
        const Vec3 normal = vertex.Normal * normalScale;
        vertex.Normal = glm::dot(normal, normal) > 0.0f ? glm::normalize(normal) : VecUtils::Up;
    }
    return true;
}

void Mesh::BuildVertexBuffer(const Array<Vertex>& InVertices, const Array<uint32_t>& InIndices) {
    Vec3 boundsMin = Vec3(0.0f);
    Vec3 boundsMax = Vec3(0.0f);
    for (int32_t i = 0; i < InVertices.Size(); i++) {
        boundsMin = i == 0 ? InVertices[i].Position : glm::min(boundsMin, InVertices[i].Position);
        boundsMax = i == 0 ? InVertices[i].Position : glm::max(boundsMax, InVertices[i].Position);
    }
    m_BoundsCenter = (boundsMin + boundsMax) * 0.5f;
    m_BoundsRadius = glm::max(glm::length(boundsMax - m_BoundsCenter), 0.001f);

    m_VertexBuffer = VertexBuffer::Create(InVertices, InIndices);
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

    BuildVertexBuffer(vertices, indices);
#else
    Array<Vertex> vertices;
    Array<uint32_t> indices;
    ImportSource(vertices, indices);

    BuildVertexBuffer(vertices, indices);
#endif

    AssetManager::Get().LoadAsset(m_Material.Get());
}

void Mesh::Unload() {
    m_VertexBuffer = nullptr;
}

void Mesh::Reimport() {
    Array<Vertex> vertices;
    Array<uint32_t> indices;
    if (!ImportSource(vertices, indices)) {
        return;
    }
    BuildVertexBuffer(vertices, indices);
}

void Mesh::Cook(ChunkedBinary& OutChunkedBinary) {
    Super::Cook(OutChunkedBinary);

    Array<Vertex> vertices;
    Array<uint32_t> indices;
    ImportSource(vertices, indices);

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

Material* Mesh::GetMaterial() const {
    return m_Material.Get();
}

void Mesh::SetMaterial(Material* InMaterial) {
    m_Material = InMaterial;
    AssetManager::Get().LoadAsset(InMaterial);
}

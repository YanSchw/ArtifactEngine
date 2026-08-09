#include "Mesh.h"
#include "Assets/AssetManager.h"
#include "Assets/Material.h"
#include "Rendering/Vertex.h"
#include "Rendering/VertexBuffer.h"
#include "Core/EngineConfig.h"
#include "Serialization/ChunkedBinary.h"
#include "Serialization/Assets/MeshLoader.h"

#include <cmath>
#include <unordered_map>

namespace {

struct WeldKey {
    int64_t X = 0;
    int64_t Y = 0;
    int64_t Z = 0;

    bool operator==(const WeldKey& InOther) const {
        return X == InOther.X && Y == InOther.Y && Z == InOther.Z;
    }
};

struct WeldKeyHasher {
    size_t operator()(const WeldKey& InKey) const {
        return std::hash<int64_t>()(InKey.X) ^ (std::hash<int64_t>()(InKey.Y) << 1)
             ^ (std::hash<int64_t>()(InKey.Z) << 2);
    }
};

WeldKey MakeWeldKey(const Vec3& InPosition) {
    constexpr float grid = 10000.0f;
    return { (int64_t)std::llround(InPosition.x * grid),
             (int64_t)std::llround(InPosition.y * grid),
             (int64_t)std::llround(InPosition.z * grid) };
}

Vec3 GetFaceNormal(const Array<Vertex>& InVertices, const Array<uint32_t>& InIndices, int32_t InTriangle) {
    const Vec3& a = InVertices[InIndices[InTriangle]].Position;
    const Vec3& b = InVertices[InIndices[InTriangle + 1]].Position;
    const Vec3& c = InVertices[InIndices[InTriangle + 2]].Position;
    return glm::cross(b - a, c - a);
}

bool HasNormals(const Array<Vertex>& InVertices) {
    for (const Vertex& vertex : InVertices) {
        if (glm::dot(vertex.Normal, vertex.Normal) > 0.0f) {
            return true;
        }
    }
    return false;
}

const float g_CreaseThreshold = glm::cos(glm::radians(60.0f));

/** Face normals, area weighted by the cross product's own length, averaged over every face meeting
 *  at a position. A face that disagrees with that average by more than the crease angle is an edge
 *  the model means to keep, so it stays on its own normal and a cube stays a cube. */
void GenerateSmoothNormals(Array<Vertex>& OutVertices, const Array<uint32_t>& InIndices) {
    std::unordered_map<WeldKey, Vec3, WeldKeyHasher> welded;

    for (int32_t i = 0; i + 2 < InIndices.Size(); i += 3) {
        const Vec3 faceNormal = GetFaceNormal(OutVertices, InIndices, i);
        for (int32_t corner = 0; corner < 3; corner++) {
            const WeldKey key = MakeWeldKey(OutVertices[InIndices[i + corner]].Position);
            welded.try_emplace(key, Vec3(0.0f)).first->second += faceNormal;
        }
    }

    for (int32_t i = 0; i + 2 < InIndices.Size(); i += 3) {
        const Vec3 faceNormal = GetFaceNormal(OutVertices, InIndices, i);
        const Vec3 faceDirection = glm::dot(faceNormal, faceNormal) > 0.0f
            ? glm::normalize(faceNormal)
            : VecUtils::Up;

        for (int32_t corner = 0; corner < 3; corner++) {
            Vertex& vertex = OutVertices[InIndices[i + corner]];
            const Vec3 accumulated = welded.at(MakeWeldKey(vertex.Position));
            const Vec3 smoothed = glm::dot(accumulated, accumulated) > 0.0f
                ? glm::normalize(accumulated)
                : faceDirection;

            vertex.Normal = glm::dot(smoothed, faceDirection) >= g_CreaseThreshold ? smoothed : faceDirection;
        }
    }
}

} // namespace

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

    if (m_NormalMode == MeshNormalMode::Smooth || !HasNormals(OutVertices)) {
        GenerateSmoothNormals(OutVertices, OutIndices);
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
    m_BoundsExtents = (boundsMax - boundsMin) * 0.5f;
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

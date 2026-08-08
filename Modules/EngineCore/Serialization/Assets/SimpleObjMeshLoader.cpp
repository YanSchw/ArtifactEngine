#include "SimpleObjMeshLoader.h"
#include "Rendering/Vertex.h"

#include <fstream>
#include <sstream>
#include <unordered_map>

namespace {

struct ObjVertexKey {
    int PositionIndex = -1;
    int TexCoordIndex = -1;
    int NormalIndex = -1;

    bool operator==(const ObjVertexKey& Other) const {
        return PositionIndex == Other.PositionIndex &&
               TexCoordIndex == Other.TexCoordIndex &&
               NormalIndex == Other.NormalIndex;
    }
};

struct ObjVertexKeyHasher {
    size_t operator()(const ObjVertexKey& Key) const {
        return std::hash<int>()(Key.PositionIndex) ^
               (std::hash<int>()(Key.TexCoordIndex) << 1) ^
               (std::hash<int>()(Key.NormalIndex) << 2);
    }
};

// Area-weighted face normals, for sources that carry none.
static void GenerateNormals(Array<Vertex>& OutVertices, const Array<uint32_t>& InIndices) {
    for (Vertex& vertex : OutVertices) {
        vertex.Normal = Vec3(0.0f);
    }

    for (int32_t i = 0; i + 2 < InIndices.Size(); i += 3) {
        Vertex& a = OutVertices[InIndices[i]];
        Vertex& b = OutVertices[InIndices[i + 1]];
        Vertex& c = OutVertices[InIndices[i + 2]];

        const Vec3 faceNormal = glm::cross(b.Position - a.Position, c.Position - a.Position);
        a.Normal += faceNormal;
        b.Normal += faceNormal;
        c.Normal += faceNormal;
    }

    for (Vertex& vertex : OutVertices) {
        vertex.Normal = glm::length(vertex.Normal) > 0.0f ? glm::normalize(vertex.Normal) : VecUtils::Up;
    }
}

static int ResolveObjIndex(int Index, int Count) {
    if (Index > 0) {
        return Index - 1;
    }

    if (Index < 0) {
        return Count + Index;
    }

    return -1;
}

static void ParseFaceVertex(const String& Token, int& OutPositionIndex, int& OutTexCoordIndex, int& OutNormalIndex) {
    OutPositionIndex = -1;
    OutTexCoordIndex = -1;
    OutNormalIndex = -1;

    std::stringstream Stream(Token);

    String Part;

    if (std::getline(Stream, Part, '/')) {
        if (!Part.empty()) {
            OutPositionIndex = std::stoi(Part);
        }
    }

    if (std::getline(Stream, Part, '/')) {
        if (!Part.empty()) {
            OutTexCoordIndex = std::stoi(Part);
        }
    }

    if (std::getline(Stream, Part, '/')) {
        if (!Part.empty()) {
            OutNormalIndex = std::stoi(Part);
        }
    }
}

}

bool SimpleObjMeshLoader::LoadMeshFromFile(const String& InFilePath, Array<Vertex>& OutVertices, Array<uint32_t>& OutIndices) {
    OutVertices.Clear();
    OutIndices.Clear();

    std::ifstream File(InFilePath);

    if (!File.is_open()) {
        return false;
    }

    Array<Vec3> Positions;
    Array<Vec2> TexCoords;
    Array<Vec3> Normals;

    std::unordered_map<ObjVertexKey, uint32_t, ObjVertexKeyHasher> VertexMap;

    String Line;

    while (std::getline(File, Line)) {
        if (Line.empty()) {
            continue;
        }

        std::stringstream Stream(Line);

        String Prefix;
        Stream >> Prefix;

        if (Prefix.empty() || Prefix[0] == '#') {
            continue;
        }

        if (Prefix == "v") {
            Vec3 Position;
            Stream >> Position.x >> Position.y >> Position.z;

            Positions.Add(Position);
        } else if (Prefix == "vt") {
            Vec2 TexCoord;
            Stream >> TexCoord.x >> TexCoord.y;

            TexCoord.y = 1.0f - TexCoord.y;

            TexCoords.Add(TexCoord);
        } else if (Prefix == "vn") {
            Vec3 Normal;
            Stream >> Normal.x >> Normal.y >> Normal.z;

            Normals.Add(Normal);
        } else if (Prefix == "f") {
            Array<String> FaceVertices;

            String Token;
            while (Stream >> Token) {
                FaceVertices.Add(Token);
            }

            if (FaceVertices.Size() < 3) {
                continue;
            }

            Array<uint32_t> FaceIndices;

            for (const String& FaceVertex : FaceVertices) {
                int PositionIndex;
                int TexCoordIndex;
                int NormalIndex;

                ParseFaceVertex(FaceVertex, PositionIndex, TexCoordIndex, NormalIndex);

                PositionIndex = ResolveObjIndex(PositionIndex, static_cast<int>(Positions.Size()));
                TexCoordIndex = ResolveObjIndex(TexCoordIndex, static_cast<int>(TexCoords.Size()));
                NormalIndex = ResolveObjIndex(NormalIndex, static_cast<int>(Normals.Size()));

                if (PositionIndex < 0 || PositionIndex >= static_cast<int>(Positions.Size())) {
                    return false;
                }

                ObjVertexKey Key;
                Key.PositionIndex = PositionIndex;
                Key.TexCoordIndex = TexCoordIndex;
                Key.NormalIndex = NormalIndex;

                auto ExistingVertex = VertexMap.find(Key);

                if (ExistingVertex != VertexMap.end()) {
                    FaceIndices.Add(ExistingVertex->second);
                    continue;
                }

                Vertex VertexData{};
                VertexData.Position = Positions[PositionIndex];
                VertexData.Color = Vec3(1.0f, 1.0f, 1.0f);

                if (TexCoordIndex >= 0 && TexCoordIndex < static_cast<int>(TexCoords.Size())) {
                    VertexData.TexCoord = TexCoords[TexCoordIndex];
                } else {
                    VertexData.TexCoord = Vec2(0.0f, 0.0f);
                }

                if (NormalIndex >= 0 && NormalIndex < static_cast<int>(Normals.Size())) {
                    VertexData.Normal = Normals[NormalIndex];
                }

                uint32_t NewVertexIndex = static_cast<uint32_t>(OutVertices.Size());

                OutVertices.Add(VertexData);
                VertexMap.emplace(Key, NewVertexIndex);

                FaceIndices.Add(NewVertexIndex);
            }

            // Fan triangulation:
            // 0 1 2 3 -> (0,1,2) (0,2,3)
            // Works for triangles, quads and n-gons.
            for (size_t i = 1; i + 1 < FaceIndices.Size(); ++i) {
                OutIndices.Add(FaceIndices[0]);
                OutIndices.Add(FaceIndices[i]);
                OutIndices.Add(FaceIndices[i + 1]);
            }
        }
    }

    if (Normals.IsEmpty()) {
        GenerateNormals(OutVertices, OutIndices);
    }

    return !OutVertices.IsEmpty() && !OutIndices.IsEmpty();
}
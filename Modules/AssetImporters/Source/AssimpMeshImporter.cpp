#include "AssimpMeshImporter.h"
#include "Rendering/Vertex.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

static Vec3 ToVec3(const aiVector3D& InVector) {
    return Vec3(InVector.x, InVector.y, InVector.z);
}

bool AssimpMeshImporter::CanImport(const String& InExtension) const {
    return Assimp::Importer().IsExtensionSupported(InExtension);
}

bool AssimpMeshImporter::Import(const String& InFilePath, Array<Vertex>& OutVertices, Array<uint32_t>& OutIndices) {
    OutVertices.Clear();
    OutIndices.Clear();

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(InFilePath,
        aiProcess_Triangulate | aiProcess_SortByPType | aiProcess_JoinIdenticalVertices |
        aiProcess_PreTransformVertices | aiProcess_FlipUVs);
    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) {
        return false;
    }

    for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++) {
        const aiMesh* mesh = scene->mMeshes[meshIndex];
        if (mesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE) {
            continue;
        }

        const uint32_t baseVertex = (uint32_t)OutVertices.Size();
        for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex{};
            vertex.Position = ToVec3(mesh->mVertices[i]);
            vertex.Color = mesh->HasVertexColors(0)
                ? Vec3(mesh->mColors[0][i].r, mesh->mColors[0][i].g, mesh->mColors[0][i].b)
                : Vec3(1.0f);
            vertex.TexCoord = mesh->HasTextureCoords(0)
                ? Vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y)
                : Vec2(0.0f);
            vertex.Normal = mesh->HasNormals() ? ToVec3(mesh->mNormals[i]) : Vec3(0.0f);
            OutVertices.Add(vertex);
        }

        for (uint32_t i = 0; i < mesh->mNumFaces; i++) {
            const aiFace& face = mesh->mFaces[i];
            for (uint32_t corner = 0; corner < face.mNumIndices; corner++) {
                OutIndices.Add(baseVertex + face.mIndices[corner]);
            }
        }
    }

    return !OutVertices.IsEmpty() && !OutIndices.IsEmpty();
}

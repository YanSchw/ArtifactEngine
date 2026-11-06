#include "AssimpCollectionImporter.h"

#include <assimp/DefaultIOSystem.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

namespace {

/** A source is often more than one file (.gltf + .bin, .obj + .mtl), and every one of them is
 *  needed again whenever the source is reimported. */
class RecordingIOSystem : public Assimp::IOSystem {
public:
    explicit RecordingIOSystem(Array<String>& OutFiles) : m_Files(OutFiles) {}

    bool Exists(const char* InFile) const override { return m_Default.Exists(InFile); }
    char getOsSeparator() const override { return m_Default.getOsSeparator(); }
    bool ComparePaths(const char* InA, const char* InB) const override { return m_Default.ComparePaths(InA, InB); }
    void Close(Assimp::IOStream* InStream) override { m_Default.Close(InStream); }

    Assimp::IOStream* Open(const char* InFile, const char* InMode) override {
        Assimp::IOStream* stream = m_Default.Open(InFile, InMode);
        const String path = fs::absolute(InFile).lexically_normal().string();
        if (stream && !m_Files.Contains(path)) {
            m_Files.Add(path);
        }
        return stream;
    }

private:
    Array<String>& m_Files;
    Assimp::DefaultIOSystem m_Default;
};

String UniqueName(const String& InName, const String& InFallback, Array<String>& InOutTaken) {
    const String base = InName.empty() ? InFallback : InName;
    String name = base;
    for (int32_t suffix = 2; InOutTaken.Contains(name); suffix++) {
        name = base + std::to_string(suffix);
    }
    InOutTaken.Add(name);
    return name;
}

Vec3 ToVec3(const aiVector3D& InVector) {
    return Vec3(InVector.x, InVector.y, InVector.z);
}

struct SceneReader {
    const aiScene& Scene;
    fs::path SourceDir;
    ImportedCollection& Out;
    Array<String> TextureKeys;
    Array<String> TextureNames;
    Array<String> MeshNames;

    int32_t AddTexture(const String& InPath, const String& InFallbackName) {
        String key = InPath;
        std::replace(key.begin(), key.end(), '\\', '/');
        if (const int32_t existing = TextureKeys.IndexOf(key); existing >= 0) {
            return existing;
        }

        ImportedTexture texture;
        if (const aiTexture* embedded = Scene.GetEmbeddedTexture(InPath.c_str())) {
            if (embedded->mHeight != 0) {
                AE_WARN("Skipping the uncompressed embedded texture '{0}'", InPath);
                return -1;
            }
            const String fileName = embedded->mFilename.length > 0 ? fs::path(embedded->mFilename.C_Str()).stem().string() : String();
            texture.Name = UniqueName(fileName, InFallbackName, TextureNames);
            texture.EmbeddedData.Resize(embedded->mWidth);
            std::memcpy(texture.EmbeddedData.Data(), embedded->pcData, embedded->mWidth);
            texture.EmbeddedExtension = "." + String(embedded->achFormatHint);
        } else {
            fs::path file = fs::path(key).is_absolute() ? fs::path(key) : SourceDir / key;
            // Paths written on the artist's machine rarely exist anywhere else; the file next to the source often does.
            if (!fs::exists(file)) {
                file = SourceDir / fs::path(key).filename();
            }
            if (!fs::exists(file)) {
                AE_WARN("Texture '{0}' was not found next to the source", InPath);
                return -1;
            }
            texture.Name = UniqueName(file.stem().string(), InFallbackName, TextureNames);
            texture.FilePath = file.lexically_normal().string();
            if (!Out.SourceFiles.Contains(texture.FilePath)) {
                Out.SourceFiles.Add(texture.FilePath);
            }
        }

        TextureKeys.Add(key);
        Out.Textures.Add(texture);
        return Out.Textures.Size() - 1;
    }

    void ReadMaterials() {
        Array<String> names;
        for (uint32_t i = 0; i < Scene.mNumMaterials; i++) {
            const aiMaterial& source = *Scene.mMaterials[i];

            ImportedMaterial material;
            material.Name = UniqueName(source.GetName().C_Str(), "Material", names);

            aiColor4D color;
            if (source.Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS || source.Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
                material.BaseColor = Vec4(color.r, color.g, color.b, color.a);
            }

            aiString texturePath;
            if (source.GetTexture(aiTextureType_BASE_COLOR, 0, &texturePath) == AI_SUCCESS ||
                source.GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == AI_SUCCESS) {
                material.BaseColorTexture = AddTexture(texturePath.C_Str(), material.Name + "_BaseColor");
            }
            Out.Materials.Add(material);
        }
    }

    void AppendSubMesh(const aiMesh& InMesh, const aiMatrix4x4& InTransform, ImportedMesh& OutMesh) const {
        const aiMatrix3x3 normalMatrix = aiMatrix3x3(InTransform).Inverse().Transpose();
        const uint32_t baseVertex = (uint32_t)OutMesh.Vertices.Size();
        for (uint32_t i = 0; i < InMesh.mNumVertices; i++) {
            Vertex vertex{};
            vertex.Position = ToVec3(InTransform * InMesh.mVertices[i]);
            vertex.Color = InMesh.HasVertexColors(0)
                ? Vec3(InMesh.mColors[0][i].r, InMesh.mColors[0][i].g, InMesh.mColors[0][i].b)
                : Vec3(1.0f);
            vertex.TexCoord = InMesh.HasTextureCoords(0)
                ? Vec2(InMesh.mTextureCoords[0][i].x, InMesh.mTextureCoords[0][i].y)
                : Vec2(0.0f);
            vertex.Normal = InMesh.HasNormals() ? ToVec3(normalMatrix * InMesh.mNormals[i]) : Vec3(0.0f);
            OutMesh.Vertices.Add(vertex);
        }

        SubMesh subMesh;
        subMesh.Name = Out.Materials[InMesh.mMaterialIndex].Name;
        subMesh.FirstIndex = (uint32_t)OutMesh.Indices.Size();
        for (uint32_t i = 0; i < InMesh.mNumFaces; i++) {
            const aiFace& face = InMesh.mFaces[i];
            for (uint32_t corner = 0; corner < face.mNumIndices; corner++) {
                OutMesh.Indices.Add(baseVertex + face.mIndices[corner]);
            }
        }
        subMesh.IndexCount = (uint32_t)OutMesh.Indices.Size() - subMesh.FirstIndex;
        OutMesh.SubMeshes.Add(subMesh);
    }

    /** One mesh per node with geometry, baked into the space of the scene root. */
    void ReadNode(const aiNode& InNode, const aiMatrix4x4& InParentTransform) {
        const aiMatrix4x4 transform = InParentTransform * InNode.mTransformation;

        ImportedMesh mesh;
        for (uint32_t i = 0; i < InNode.mNumMeshes; i++) {
            const aiMesh& source = *Scene.mMeshes[InNode.mMeshes[i]];
            if ((source.mPrimitiveTypes & ~aiPrimitiveType_NGONEncodingFlag) == aiPrimitiveType_TRIANGLE) {
                AppendSubMesh(source, transform, mesh);
            }
        }
        if (!mesh.Indices.IsEmpty()) {
            Out.Meshes.Add(ImportedMesh::Combine({ mesh }, UniqueName(InNode.mName.C_Str(), "Mesh", MeshNames)));
        }

        for (uint32_t i = 0; i < InNode.mNumChildren; i++) {
            ReadNode(*InNode.mChildren[i], transform);
        }
    }
};

}

bool AssimpCollectionImporter::CanImport(const String& InExtension) const {
    return Assimp::Importer().IsExtensionSupported(InExtension);
}

bool AssimpCollectionImporter::Import(const String& InFilePath, ImportedCollection& OutCollection) {
    OutCollection = ImportedCollection();

    Assimp::Importer importer;
    importer.SetIOHandler(new RecordingIOSystem(OutCollection.SourceFiles));
    const aiScene* scene = importer.ReadFile(InFilePath,
        aiProcess_Triangulate | aiProcess_SortByPType | aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs);
    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
        AE_WARN("Assimp could not read '{0}': {1}", InFilePath, importer.GetErrorString());
        return false;
    }

    SceneReader reader{ *scene, fs::path(InFilePath).parent_path(), OutCollection, {}, {}, {} };
    reader.ReadMaterials();
    reader.ReadNode(*scene->mRootNode, aiMatrix4x4());
    return true;
}

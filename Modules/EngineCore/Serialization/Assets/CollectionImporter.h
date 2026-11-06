#pragma once
#include "MeshImporter.h"
#include "Common/Types.h"
#include "CollectionImporter.gen.h"

struct ImportedMaterial {
    String Name;
    Vec4 BaseColor = Vec4(1.0f);
    /** Index into ImportedCollection::Textures, -1 for none. */
    int32_t BaseColorTexture = -1;
};

/** Either a file next to the source, or bytes the source embeds. */
struct ImportedTexture {
    String Name;
    String FilePath;
    Array<byte> EmbeddedData;
    String EmbeddedExtension;
};

struct ImportedCollection {
    Array<ImportedMesh> Meshes;
    Array<ImportedMaterial> Materials;
    Array<ImportedTexture> Textures;
    /** Every file the import read, the source itself included. */
    Array<String> SourceFiles;
};

/** Reads a whole scene: one mesh per geometry object, the materials they use and their textures. */
class CollectionImporter : public AssetImporter {
public:
    ARTIFACT_CLASS();

    virtual bool Import(const String& InFilePath, ImportedCollection& OutCollection) = 0;
};

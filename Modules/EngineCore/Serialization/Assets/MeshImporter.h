#pragma once
#include "AssetImporter.h"
#include "Assets/Mesh.h"
#include "Common/Array.h"
#include "Rendering/Vertex.h"
#include "MeshImporter.gen.h"

/** One geometry object of a source file, with a sub-mesh per material it uses. */
struct ImportedMesh {
    String Name;
    Array<Vertex> Vertices;
    Array<uint32_t> Indices;
    Array<SubMesh> SubMeshes;

    /** All meshes as one, sub-meshes sharing a name merged into a single slot. */
    static ImportedMesh Combine(const Array<ImportedMesh>& InMeshes, const String& InName);
};

class MeshImporter : public AssetImporter {
public:
    ARTIFACT_CLASS();

    /** Everything in the file as one mesh. */
    virtual bool Import(const String& InFilePath, ImportedMesh& OutMesh) = 0;
};

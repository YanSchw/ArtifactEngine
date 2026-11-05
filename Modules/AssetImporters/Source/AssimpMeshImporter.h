#pragma once
#include "Serialization/Assets/MeshImporter.h"
#include "AssimpMeshImporter.gen.h"

class AssimpMeshImporter : public MeshImporter {
public:
    ARTIFACT_CLASS();

    virtual bool CanImport(const String& InExtension) const override;
    virtual int32_t GetPriority() const override { return -1; }
    virtual bool Import(const String& InFilePath, Array<Vertex>& OutVertices, Array<uint32_t>& OutIndices) override;
};

#pragma once
#include "Serialization/Assets/MeshImporter.h"
#include "ObjMeshImporter.gen.h"

class ObjMeshImporter : public MeshImporter {
public:
    ARTIFACT_CLASS();

    virtual bool CanImport(const String& InExtension) const override;
    virtual bool Import(const String& InFilePath, ImportedMesh& OutMesh) override;
};

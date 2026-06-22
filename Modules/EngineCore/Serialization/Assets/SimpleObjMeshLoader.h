#pragma once
#include "MeshLoader.h"
#include "SimpleObjMeshLoader.gen.h"

class SimpleObjMeshLoader : public MeshLoader {
public:
    ARTIFACT_CLASS();

    virtual bool LoadMeshFromFile(const String& InFilePath, Array<Vertex>& OutVertices, Array<uint32_t>& OutIndices) override;
};
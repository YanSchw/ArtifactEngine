#pragma once
#include "Common/Array.h"
#include "Object/Object.h"
#include "MeshLoader.gen.h"

struct Vertex;

class MeshLoader : public Object {
public:
    ARTIFACT_CLASS();

    virtual bool LoadMeshFromFile(const String& InFilePath, Array<Vertex>& OutVertices, Array<uint32_t>& OutIndices) = 0;
};
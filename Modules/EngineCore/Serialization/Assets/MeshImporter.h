#pragma once
#include "AssetImporter.h"
#include "Common/Array.h"
#include "MeshImporter.gen.h"

struct Vertex;

class MeshImporter : public AssetImporter {
public:
    ARTIFACT_CLASS();

    virtual bool Import(const String& InFilePath, Array<Vertex>& OutVertices, Array<uint32_t>& OutIndices) = 0;
};

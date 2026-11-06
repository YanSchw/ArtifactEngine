#pragma once
#include "Serialization/Assets/CollectionImporter.h"
#include "AssimpCollectionImporter.gen.h"

class AssimpCollectionImporter : public CollectionImporter {
public:
    ARTIFACT_CLASS();

    virtual bool CanImport(const String& InExtension) const override;
    virtual bool Import(const String& InFilePath, ImportedCollection& OutCollection) override;
};

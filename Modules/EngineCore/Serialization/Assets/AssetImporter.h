#pragma once
#include "Object/Object.h"
#include "AssetImporter.gen.h"

/** Turns a source file into engine data. Implementations live in the dev-only AssetImporters
 *  module and are found through reflection, so packaged builds ship none of them. */
class AssetImporter : public Object {
public:
    ARTIFACT_CLASS();

    virtual bool CanImport(const String& InExtension) const = 0;
    virtual int32_t GetPriority() const { return 0; }

    template<typename T>
    static T* Find(const String& InFilePath) {
        return Cast<T>(Find(T::StaticClass(), InFilePath));
    }

    static AssetImporter* Find(const Class& InImporterClass, const String& InFilePath);
};

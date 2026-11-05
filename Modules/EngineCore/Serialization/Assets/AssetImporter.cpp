#include "AssetImporter.h"
#include "Object/Pointer.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

static const Array<SharedObjectPtr<AssetImporter>>& GetImporters() {
    static const Array<SharedObjectPtr<AssetImporter>> s_Importers = [] {
        Array<SharedObjectPtr<AssetImporter>> importers;
        for (const Class& importerClass : Class::GetSubclassesOf(AssetImporter::StaticClass())) {
            if (AssetImporter* importer = Cast<AssetImporter>(Object::Create(importerClass))) {
                importers.Add(importer);
            }
        }
        return importers.Sort([](const SharedObjectPtr<AssetImporter>& InA, const SharedObjectPtr<AssetImporter>& InB) {
            return InA->GetPriority() > InB->GetPriority();
        });
    }();
    return s_Importers;
}

AssetImporter* AssetImporter::Find(const Class& InImporterClass, const String& InFilePath) {
    String extension = std::filesystem::path(InFilePath).extension().string();
    std::ranges::transform(extension, extension.begin(), [](unsigned char InChar) { return (char)std::tolower(InChar); });

    for (const SharedObjectPtr<AssetImporter>& importer : GetImporters()) {
        if (importer->IsA(InImporterClass) && importer->CanImport(extension)) {
            return importer.Get();
        }
    }
    return nullptr;
}

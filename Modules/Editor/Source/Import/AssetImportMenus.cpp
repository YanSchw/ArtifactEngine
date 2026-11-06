#include "AssetImportMenus.h"
#include "Assets/AudioClip.h"
#include "Assets/CollectionAsset.h"
#include "Assets/Font.h"
#include "Assets/Mesh.h"
#include "Assets/Texture2D.h"
#include "Assets/VectorImage.h"
#include "Object/Pointer.h"
#include "Serialization/Assets/AudioClipImporter.h"
#include "Serialization/Assets/CollectionImporter.h"
#include "Serialization/Assets/FontImporter.h"
#include "Serialization/Assets/Texture2DImporter.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

/** A scene importer knows every other file a model reads, whatever the model is imported as. */
static Array<String> GetSceneSourceFiles(const String& InFilePath) {
    ImportedCollection collection;
    CollectionImporter* importer = AssetImporter::Find<CollectionImporter>(InFilePath);
    if (importer && importer->Import(InFilePath, collection)) {
        return collection.SourceFiles;
    }
    return { InFilePath };
}

Array<AssetImportMenu*> AssetImportMenu::FindFor(const String& InFilePath) {
    static const Array<SharedObjectPtr<AssetImportMenu>> s_Menus = [] {
        Array<SharedObjectPtr<AssetImportMenu>> menus;
        for (const Class& menuClass : Class::GetSubclassesOf(StaticClass())) {
            if (AssetImportMenu* menu = Cast<AssetImportMenu>(Object::Create(menuClass))) {
                menus.Add(menu);
            }
        }
        return menus.Sort([](const SharedObjectPtr<AssetImportMenu>& InA, const SharedObjectPtr<AssetImportMenu>& InB) {
            return InA->GetPriority() > InB->GetPriority();
        });
    }();

    Array<AssetImportMenu*> found;
    for (const SharedObjectPtr<AssetImportMenu>& menu : s_Menus) {
        if (menu->CanImport(InFilePath)) {
            found.Add(menu.Get());
        }
    }
    return found;
}

Class MeshImportMenu::GetAssetClass() const {
    return Mesh::StaticClass();
}

bool MeshImportMenu::CanImport(const String& InFilePath) const {
    return AssetImporter::Find<MeshImporter>(InFilePath) != nullptr;
}

Array<String> MeshImportMenu::GetSourceFiles(const String& InFilePath) const {
    return GetSceneSourceFiles(InFilePath);
}

Class CollectionImportMenu::GetAssetClass() const {
    return CollectionAsset::StaticClass();
}

bool CollectionImportMenu::CanImport(const String& InFilePath) const {
    return AssetImporter::Find<CollectionImporter>(InFilePath) != nullptr;
}

Array<String> CollectionImportMenu::GetSourceFiles(const String& InFilePath) const {
    return GetSceneSourceFiles(InFilePath);
}

void CollectionImportMenu::OnImported(Asset& InAsset) const {
    if (CollectionAsset* collection = Cast<CollectionAsset>(&InAsset)) {
        collection->Reimport();
    }
}

Class Texture2DImportMenu::GetAssetClass() const {
    return Texture2D::StaticClass();
}

bool Texture2DImportMenu::CanImport(const String& InFilePath) const {
    return AssetImporter::Find<Texture2DImporter>(InFilePath) != nullptr;
}

Class VectorImageImportMenu::GetAssetClass() const {
    return VectorImage::StaticClass();
}

bool VectorImageImportMenu::CanImport(const String& InFilePath) const {
    String extension = std::filesystem::path(InFilePath).extension().string();
    std::ranges::transform(extension, extension.begin(), [](unsigned char InChar) { return (char)std::tolower(InChar); });
    return extension == ".svg";
}

Class FontImportMenu::GetAssetClass() const {
    return Font::StaticClass();
}

bool FontImportMenu::CanImport(const String& InFilePath) const {
    return AssetImporter::Find<FontImporter>(InFilePath) != nullptr;
}

Class AudioClipImportMenu::GetAssetClass() const {
    return AudioClip::StaticClass();
}

bool AudioClipImportMenu::CanImport(const String& InFilePath) const {
    return AssetImporter::Find<AudioClipImporter>(InFilePath) != nullptr;
}

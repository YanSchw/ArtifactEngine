#include "AssimpMeshImporter.h"
#include "AssimpCollectionImporter.h"

#include <assimp/Importer.hpp>

#include <filesystem>

bool AssimpMeshImporter::CanImport(const String& InExtension) const {
    return Assimp::Importer().IsExtensionSupported(InExtension);
}

bool AssimpMeshImporter::Import(const String& InFilePath, ImportedMesh& OutMesh) {
    ImportedCollection collection;
    if (!AssimpCollectionImporter().Import(InFilePath, collection)) {
        return false;
    }
    OutMesh = ImportedMesh::Combine(collection.Meshes, std::filesystem::path(InFilePath).stem().string());
    return !OutMesh.Indices.IsEmpty();
}

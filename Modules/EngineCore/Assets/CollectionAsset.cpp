#include "CollectionAsset.h"
#include "AssetManager.h"
#include "Material.h"
#include "ShaderGraph.h"
#include "Texture2D.h"
#include "Core/EngineConfig.h"
#include "Graph/NodeGraph.h"
#include "Graph/ShaderGraphNodes.h"
#include "Platform/FileIO.h"
#include "Serialization/Assets/CollectionImporter.h"

#include <cctype>
#include <filesystem>

namespace fs = std::filesystem;

static constexpr const char* s_WhiteTextureId = "d3cebd91-4419-4b23-b643-a5f23642eec1";

static String ToFileName(const String& InName) {
    String name;
    for (char c : fs::path(InName).filename().string()) {
        name += std::isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' ? c : '_';
    }
    return name;
}

/** The imported color as the BaseColor input, times a BaseColorMap input when there is a texture,
 *  wired into the template's BaseColor. */
static void BuildSurfaceGraph(ShaderGraph& OutGraph, const String& InTemplatePath, const Vec4& InBaseColor, Texture2D* InBaseColorMap) {
    OutGraph.Reset(InTemplatePath);
    NodeGraph* graph = OutGraph.GetGraph();

    GraphNode* output = nullptr;
    for (const SharedObjectPtr<GraphNode>& node : graph->Nodes) {
        output = Cast<ShaderGraphOutputNode>(node.Get()) ? node.Get() : output;
    }
    GraphPin* target = output ? output->FindPin("BaseColor", GraphPinDirection::Input) : nullptr;
    if (!target) {
        AE_WARN("The template '{0}' has no BaseColor for '{1}' to import into", InTemplatePath, OutGraph.GetDisplayName());
        return;
    }

    ShaderGraphValueNode* color = graph->CreateNode<ShaderGraphValueNode>(Vec2(-440.0f, 160.0f));
    color->ValueType = ShaderValueType::Color;
    color->Value = InBaseColor;
    color->ExposeAsInput = true;
    color->InputName = "BaseColor";
    color->SyncPins();

    GraphNode* result = color;
    GraphPin* resultPin = color->FindPin("Value", GraphPinDirection::Output);
    if (InBaseColorMap) {
        ShaderGraphValueNode* map = graph->CreateNode<ShaderGraphValueNode>(Vec2(-440.0f, -60.0f));
        map->ValueType = ShaderValueType::Texture2D;
        map->Texture = InBaseColorMap;
        map->ExposeAsInput = true;
        map->InputName = "BaseColorMap";
        map->SyncPins();

        ShaderGraphBinaryNode* multiply = graph->CreateNode<ShaderGraphBinaryNode>(Vec2(-120.0f, 40.0f));
        multiply->Operation = ShaderBinaryOp::Multiply;
        graph->Connect(*map, *map->FindPin("RGBA", GraphPinDirection::Output), *multiply, *multiply->FindPin("A", GraphPinDirection::Input));
        graph->Connect(*color, *resultPin, *multiply, *multiply->FindPin("B", GraphPinDirection::Input));
        result = multiply;
        resultPin = multiply->FindPin("Result", GraphPinDirection::Output);
    }
    graph->Connect(*result, *resultPin, *output, *target);
}

static void Compile(ShaderGraph& InGraph) {
    if (!InGraph.IsLoaded()) {
        AssetManager::Get().LoadAsset(&InGraph);
        return;
    }
    String error;
    if (!InGraph.Recompile(error)) {
        AE_ERROR("ShaderGraph '{0}' failed to compile:\n{1}", InGraph.GetDisplayName(), error);
    }
}

template<typename T>
T* CollectionAsset::FindOrAddSubAsset(const String& InKey, const String& InName, bool* OutCreated) {
    const UUID id = UUID::Derive(GetId(), InKey);
    Asset* existing = AssetManager::Get().GetAsset(id);
    // Changing how materials are created swaps the class but keeps the id, so references survive.
    if (existing && existing->GetClass() != T::StaticClass()) {
        AssetManager::Get().DeleteAsset(existing);
        existing = nullptr;
    }
    if (OutCreated) {
        *OutCreated = existing == nullptr;
    }
    return existing ? Cast<T>(existing) : Cast<T>(AssetManager::Get().AddSubAsset(Object::Create<T>(), this, id, InName));
}

const ImportedCollection* CollectionAsset::GetImported() {
    if (m_Imported) {
        return m_Imported.get();
    }

    const String path = EngineConfig::ResolveContentPath(m_SourcePath);
    CollectionImporter* importer = AssetImporter::Find<CollectionImporter>(path);
    auto imported = std::make_shared<ImportedCollection>();
    if (!importer || !importer->Import(path, *imported)) {
        AE_WARN("Importing collection from file {0} was unsuccessful!", path);
        return nullptr;
    }
    m_Imported = imported;
    return m_Imported.get();
}

void CollectionAsset::ReleaseImportedOnceMeshesLoaded() {
    int32_t unloaded = 0;
    for (Asset* subAsset : AssetManager::Get().GetSubAssets(this)) {
        unloaded += Cast<Mesh>(subAsset) && !subAsset->IsLoaded() ? 1 : 0;
    }
    // The mesh asking right now is still unloaded, and the last one to ask.
    if (unloaded <= 1) {
        m_Imported = nullptr;
    }
}

bool CollectionAsset::ImportMesh(const String& InName, ImportedMesh& OutMesh) {
    const ImportedCollection* imported = GetImported();
    if (!imported) {
        return false;
    }

    bool found = InName.empty();
    if (found) {
        OutMesh = ImportedMesh::Combine(imported->Meshes, GetDisplayName());
    }
    for (const ImportedMesh& mesh : imported->Meshes) {
        if (!found && mesh.Name == InName) {
            OutMesh = mesh;
            found = true;
        }
    }

    if (!m_Reimporting) {
        ReleaseImportedOnceMeshesLoaded();
    }
    return found;
}

String CollectionAsset::ResolveTexture(const ImportedTexture& InTexture) const {
    if (!InTexture.FilePath.empty()) {
        const String contentPath = EngineConfig::ToContentPath(InTexture.FilePath);
        if (contentPath.empty()) {
            AE_WARN("Texture '{0}' of '{1}' is outside every content folder", InTexture.FilePath, m_SourcePath);
        }
        return contentPath;
    }

    // Embedded textures are written out next to the source, so a Texture2D can import them.
    const fs::path source = EngineConfig::ResolveContentPath(m_SourcePath);
    const fs::path file = source.parent_path() / (source.stem().string() + "_" + ToFileName(InTexture.Name) + InTexture.EmbeddedExtension);
    if (!FileIO::WriteBytesToFile(file.string(), InTexture.EmbeddedData.Data(), InTexture.EmbeddedData.Size())) {
        AE_WARN("Failed to write the embedded texture '{0}' to {1}", InTexture.Name, file.string());
        return String();
    }
    return EngineConfig::ToContentPath(file.string());
}

bool CollectionAsset::Reimport() {
    m_Imported = nullptr;
    const ImportedCollection* imported = GetImported();
    if (!imported) {
        return false;
    }
    m_Reimporting = true;

    const CollectionImportSettings& settings = m_ImportSettings;
    Array<Asset*> current;

    const auto importTexture = [&](int32_t InIndex) -> Texture2D* {
        if (InIndex < 0 || !settings.Textures.Import) {
            return nullptr;
        }
        const ImportedTexture& source = imported->Textures[InIndex];
        const String path = ResolveTexture(source);
        if (path.empty()) {
            return nullptr;
        }
        Texture2D* texture = FindOrAddSubAsset<Texture2D>("Texture2D:" + source.Name, source.Name);
        texture->SetSourcePath(path);
        texture->SetGenerateMips(settings.Textures.GenerateMips);
        texture->Reimport();
        if (!current.Contains(texture)) {
            current.Add(texture);
        }
        return texture;
    };

    const Array<ImportedMesh> sourceMeshes = settings.Meshes.Combine
        ? Array<ImportedMesh>({ ImportedMesh::Combine(imported->Meshes, String()) })
        : imported->Meshes;

    // Formats like OBJ always declare a default material, so only the ones geometry uses come in.
    Array<String> usedMaterials;
    for (const ImportedMesh& source : sourceMeshes) {
        for (const SubMesh& subMesh : source.SubMeshes) {
            if (!usedMaterials.Contains(subMesh.Name)) {
                usedMaterials.Add(subMesh.Name);
            }
        }
    }

    // Generated graphs are the user's once they exist: a new template is applied, nothing is rebuilt.
    const CollectionMaterialSettings& materialSettings = settings.Materials;
    const auto useTemplate = [&](ShaderGraph& InGraph) {
        if (InGraph.GetTemplatePath() != materialSettings.ShaderTemplate) {
            InGraph.SetTemplatePath(materialSettings.ShaderTemplate);
        }
        Compile(InGraph);
    };

    ShaderGraph* baseGraph = nullptr;
    if (materialSettings.CreateAs == CollectionMaterialMode::Materials) {
        baseGraph = materialSettings.BaseShaderGraph.Get();
        if (!baseGraph) {
            bool created = false;
            baseGraph = FindOrAddSubAsset<ShaderGraph>("ShaderGraph:Base", "Base", &created);
            if (created) {
                BuildSurfaceGraph(*baseGraph, materialSettings.ShaderTemplate, Vec4(1.0f),
                                  AssetManager::Get().GetAsset<Texture2D>(UUID::FromString(s_WhiteTextureId)));
            }
            useTemplate(*baseGraph);
            current.Add(baseGraph);
        }
    }

    Map<String, Material*> materials;
    for (const ImportedMaterial& source : imported->Materials) {
        if (materialSettings.CreateAs == CollectionMaterialMode::None || !usedMaterials.Contains(source.Name)) {
            continue;
        }
        Texture2D* baseColorMap = importTexture(source.BaseColorTexture);
        bool created = false;
        Material* material = nullptr;
        if (materialSettings.CreateAs == CollectionMaterialMode::ShaderGraphs) {
            ShaderGraph* graph = FindOrAddSubAsset<ShaderGraph>("Material:" + source.Name, source.Name, &created);
            if (created) {
                BuildSurfaceGraph(*graph, materialSettings.ShaderTemplate, source.BaseColor, baseColorMap);
            }
            useTemplate(*graph);
            material = graph;
        } else {
            material = FindOrAddSubAsset<Material>("Material:" + source.Name, source.Name, &created);
            if (created || material->GetBaseGraph() != baseGraph) {
                material->SetBaseGraph(baseGraph);
                material->SetInputValue("BaseColor", source.BaseColor);
                if (baseColorMap) {
                    material->SetInputTexture("BaseColorMap", baseColorMap);
                }
            }
        }
        materials[source.Name] = material;
        current.Add(material);
    }

    for (const ImportedMesh& source : sourceMeshes) {
        Mesh* mesh = FindOrAddSubAsset<Mesh>("Mesh:" + source.Name, source.Name.empty() ? GetDisplayName() : source.Name);
        mesh->SetSourceMesh(source.Name);
        mesh->SetImportSettings(settings.Meshes.Scale, settings.Meshes.Offset, settings.Meshes.Normals);
        for (int32_t slot = 0; slot < source.SubMeshes.Size(); slot++) {
            const String& slotName = source.SubMeshes[slot].Name;
            if (!mesh->GetAssignedMaterial(slot) && materials.ContainsKey(slotName)) {
                mesh->SetMaterial(slot, materials[slotName]);
            }
        }
        if (mesh->IsLoaded()) {
            mesh->Reimport();
        } else {
            AssetManager::Get().LoadAsset(mesh);
        }
        current.Add(mesh);
    }

    for (Asset* subAsset : AssetManager::Get().GetSubAssets(this)) {
        if (!current.Contains(subAsset)) {
            AssetManager::Get().DeleteAsset(subAsset);
        }
    }

    m_Reimporting = false;
    m_Imported = nullptr;
    return true;
}

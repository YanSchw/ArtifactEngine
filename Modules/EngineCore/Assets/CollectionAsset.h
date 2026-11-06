#pragma once
#include "Asset.h"
#include "Mesh.h"
#include "ShaderGraph.h"
#include <memory>
#include "CollectionAsset.gen.h"

struct ImportedCollection;
struct ImportedTexture;

ARTIFACT_ENUM();
enum class CollectionMaterialMode : uint8_t {
    /** A shader graph of its own per material, built on the chosen template. */
    ShaderGraphs = 0,
    /** Materials deriving from one base shader graph. */
    Materials = 1,
    None = 2
};

struct CollectionMeshSettings {
    ARTIFACT_STRUCT();

    /** One mesh with a slot per material, instead of one mesh per object in the source. */
    PROPERTY()
    bool Combine = true;

    PROPERTY()
    Vec3 Scale = Vec3(1.0f);

    PROPERTY()
    Vec3 Offset = Vec3(0.0f);

    PROPERTY()
    MeshNormalMode Normals = MeshNormalMode::Import;
};

/** Imported colors and textures land on the BaseColor and BaseColorMap inputs. */
struct CollectionMaterialSettings {
    ARTIFACT_STRUCT();

    PROPERTY()
    CollectionMaterialMode CreateAs = CollectionMaterialMode::ShaderGraphs;

    /** What the generated shader graphs are built on. */
    PROPERTY()
    String ShaderTemplate = ShaderGraph::DefaultTemplate;

    /** For Materials: the graph they derive from. Left empty, one is generated for the collection. */
    PROPERTY()
    WeakObjectPtr<ShaderGraph> BaseShaderGraph;
};

struct CollectionTextureSettings {
    ARTIFACT_STRUCT();

    /** Textures are found through materials, so none come in without them. */
    PROPERTY()
    bool Import = true;

    PROPERTY()
    bool GenerateMips = true;
};

struct CollectionImportSettings {
    ARTIFACT_STRUCT();

    PROPERTY()
    CollectionMeshSettings Meshes;

    PROPERTY()
    CollectionMaterialSettings Materials;

    PROPERTY()
    CollectionTextureSettings Textures;
};

/** A collection asset: it holds the meshes, materials and textures imported from one source
 *  file as sub-assets, and the settings to import them again. */
class CollectionAsset : public Asset {
public:
    ARTIFACT_CLASS();

    virtual String GetSourcePath() const override { return m_SourcePath; }
    virtual void SetSourcePath(const String& InPath) override { m_SourcePath = InPath; }

    const CollectionImportSettings& GetImportSettings() const { return m_ImportSettings; }
    void SetImportSettings(const CollectionImportSettings& InSettings) { m_ImportSettings = InSettings; }

    /** Sub-assets new to the source are created, existing ones keep their ids and edits, and the
     *  ones the source no longer has are removed. */
    bool Reimport();

    /** An empty name combines every mesh of the source into one. */
    bool ImportMesh(const String& InName, ImportedMesh& OutMesh);

protected:
    virtual void Load() override {}
    virtual void Unload() override {}

private:
    const ImportedCollection* GetImported();
    void ReleaseImportedOnceMeshesLoaded();
    String ResolveTexture(const ImportedTexture& InTexture) const;

    template<typename T>
    T* FindOrAddSubAsset(const String& InKey, const String& InName, bool* OutCreated = nullptr);

    PROPERTY()
    String m_SourcePath;

    PROPERTY()
    CollectionImportSettings m_ImportSettings;

    std::shared_ptr<ImportedCollection> m_Imported;
    bool m_Reimporting = false;
};

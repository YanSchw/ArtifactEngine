#pragma once
#include "Asset.h"
#include "Common/Types.h"
#include "Object/Pointer.h"
#include "Mesh.gen.h"

class Material;
class VertexBuffer;
struct ImportedMesh;

ARTIFACT_ENUM();
enum class MeshNormalMode : uint8_t {
    Import = 0,
    Smooth = 1
};

/** A range of the index buffer drawn with one material slot. */
struct SubMesh {
    String Name;
    uint32_t FirstIndex = 0;
    uint32_t IndexCount = 0;
};

class Mesh : public Asset {
public:
    ARTIFACT_CLASS();
    Mesh();
    virtual ~Mesh() = default;

protected:
    virtual void Load() override;
    virtual void Unload() override;
    virtual void Cook(class ChunkedBinary& OutChunkedBinary) override;

public:
    virtual bool IsLoaded() const override;
    virtual String GetDisplayName() const override { return IsSubAsset() ? Asset::GetDisplayName() : DisplayNameFromPath(m_MeshPath); }
    virtual String GetSourcePath() const override { return m_MeshPath; }
    virtual void SetSourcePath(const String& InPath) override { m_MeshPath = InPath; }
    VertexBuffer* GetVertexBuffer() const;

    /** One per material slot, in slot order. */
    const Array<SubMesh>& GetSubMeshes() const { return m_SubMeshes; }

    /** The slot's material, or the default material when nothing is assigned to it. */
    FUNCTION(Pure)
    Material* GetMaterial(int32_t InSlot = 0) const;
    FUNCTION(Pure)
    Material* GetAssignedMaterial(int32_t InSlot) const;
    void SetMaterial(int32_t InSlot, Material* InMaterial);

    FUNCTION(Pure)
    Vec3 GetBoundsCenter() const { return m_BoundsCenter; }
    FUNCTION(Pure)
    float GetBoundsRadius() const { return m_BoundsRadius; }
    /** Half the size of the axis-aligned bounding box around GetBoundsCenter(). */
    FUNCTION(Pure)
    Vec3 GetBoundsExtents() const { return m_BoundsExtents; }

    void SetImportSettings(const Vec3& InScale, const Vec3& InOffset, MeshNormalMode InNormalMode);
    /** Inside a collection: the source object this mesh is built from, empty for all of them combined. */
    void SetSourceMesh(const String& InName) { m_SourceMesh = InName; }

    /** Rebuilds the vertex buffer from the source file, applying the current import transform. */
    void Reimport();

private:
    bool ImportSource(ImportedMesh& OutMesh) const;
    void BuildVertexBuffer(const ImportedMesh& InMesh);

    SharedObjectPtr<VertexBuffer> m_VertexBuffer = nullptr;
    Array<SubMesh> m_SubMeshes;
    Vec3 m_BoundsCenter = Vec3(0.0f);
    Vec3 m_BoundsExtents = Vec3(1.0f);
    float m_BoundsRadius = 1.0f;

    PROPERTY()
    String m_MeshPath;

    PROPERTY()
    String m_SourceMesh;

    PROPERTY()
    Array<WeakObjectPtr<Material>> m_Materials;

    PROPERTY()
    Vec3 m_ImportScale = Vec3(1.0f);

    PROPERTY()
    Vec3 m_ImportOffset = Vec3(0.0f);

    PROPERTY()
    MeshNormalMode m_NormalMode = MeshNormalMode::Import;
};

#pragma once
#include "Asset.h"
#include "Common/Types.h"
#include "Object/Pointer.h"
#include "Mesh.gen.h"

class Material;
class VertexBuffer;
struct Vertex;

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
    virtual String GetDisplayName() const override { return DisplayNameFromPath(m_MeshPath); }
    VertexBuffer* GetVertexBuffer() const;

    Material* GetMaterial() const;
    void SetMaterial(Material* InMaterial);

    Vec3 GetBoundsCenter() const { return m_BoundsCenter; }
    float GetBoundsRadius() const { return m_BoundsRadius; }

    /** Rebuilds the vertex buffer from the source file, applying the current import transform. */
    void Reimport();

private:
    bool ImportSource(Array<Vertex>& OutVertices, Array<uint32_t>& OutIndices) const;
    void BuildVertexBuffer(const Array<Vertex>& InVertices, const Array<uint32_t>& InIndices);

    SharedObjectPtr<VertexBuffer> m_VertexBuffer = nullptr;
    Vec3 m_BoundsCenter = Vec3(0.0f);
    float m_BoundsRadius = 1.0f;

    PROPERTY()
    String m_MeshPath;

    PROPERTY()
    WeakObjectPtr<Material> m_Material;

    PROPERTY()
    Vec3 m_ImportScale = Vec3(1.0f);

    PROPERTY()
    Vec3 m_ImportOffset = Vec3(0.0f);
};

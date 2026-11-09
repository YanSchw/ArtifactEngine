#pragma once
#include "Object/Pointer.h"
#include "Node3D.h"
#include "StaticMeshNode.gen.h"

class ShaderData;
class Material;
class Mesh;

/** Mirrors the push-constant block declared by Shader.glsl. */
struct MeshShaderData {
    Mat4 WorldTransform = Mat4(1.0f);
    uint32_t NodeId = 0;
    uint32_t Padding[3] = { 0, 0, 0 };
};

class StaticMeshNode : public Node3D {
public:
    ARTIFACT_CLASS();

    StaticMeshNode();
    ~StaticMeshNode();
    ShaderData* GetPerMeshShaderData();

    Mesh* GetMesh() const;
    void SetMesh(Mesh* InMesh);

    /** The slot's override, or the mesh's own material for that slot when there is none. */
    Material* GetMaterial(int32_t InSlot = 0) const;
    Material* GetMaterialOverride(int32_t InSlot) const;
    /** Null clears the override. */
    void SetMaterialOverride(int32_t InSlot, Material* InMaterial);

    bool GetCastShadow() const { return m_CastShadow; }
    void SetCastShadow(bool InCastShadow) { m_CastShadow = InCastShadow; }

private:
    SharedObjectPtr<ShaderData> m_PerMeshShaderData;

    PROPERTY()
    WeakObjectPtr<Mesh> m_Mesh;

    PROPERTY()
    Array<WeakObjectPtr<Material>> m_MaterialOverrides;

    PROPERTY()
    bool m_CastShadow = true;
};

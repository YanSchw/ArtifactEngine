#pragma once
#include "Object/Pointer.h"
#include "Node3D.h"
#include "StaticMeshNode.gen.h"

class ShaderData;
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

private:
    SharedObjectPtr<ShaderData> m_PerMeshShaderData;

    PROPERTY()
    WeakObjectPtr<Mesh> m_Mesh;

    PROPERTY()
    bool m_CastShadow = true;
};
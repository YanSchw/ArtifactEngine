#pragma once
#include "Object/Pointer.h"
#include "Node3D.h"
#include "StaticMeshNode.gen.h"

class ShaderData;
class Mesh;

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
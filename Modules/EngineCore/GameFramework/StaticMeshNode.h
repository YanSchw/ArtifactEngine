#pragma once
#include "Object/Pointer.h"
#include "Node3D.h"
#include "StaticMeshNode.gen.h"

class ShaderData;

class StaticMeshNode : public Node3D {
public:
    ARTIFACT_CLASS();

    StaticMeshNode();
    ShaderData* GetPerMeshShaderData();

private:
    SharedObjectPtr<ShaderData> m_PerMeshShaderData;
};
#include "StaticMeshNode.h"
#include "Rendering/ShaderData.h"

StaticMeshNode::StaticMeshNode() {
    m_PerMeshShaderData = new ShaderData();
}

ShaderData* StaticMeshNode::GetPerMeshShaderData() {
    Mat4 transformMatrix = GetTransformMatrix();
    m_PerMeshShaderData->Set(transformMatrix);
    return m_PerMeshShaderData;
}
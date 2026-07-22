#include "StaticMeshNode.h"
#include "Assets/Mesh.h"
#include "Rendering/ShaderData.h"


#include "Assets/AssetManager.h"

StaticMeshNode::StaticMeshNode() {
    m_PerMeshShaderData = new ShaderData();
    SetMesh(AssetManager::Get().GetAsset<Mesh>(UUID::FromString("c6308770-3a5b-4b2b-9cec-14ba803ff817")));
}

StaticMeshNode::~StaticMeshNode() {
    m_PerMeshShaderData = nullptr;
}

ShaderData* StaticMeshNode::GetPerMeshShaderData() {
    MeshShaderData data;
    data.WorldTransform = GetTransformMatrix();
    data.NodeId = GetNodeId();
    m_PerMeshShaderData->Set(data);
    return m_PerMeshShaderData;
}

Mesh* StaticMeshNode::GetMesh() const {
    return m_Mesh;
}

void StaticMeshNode::SetMesh(Mesh* InMesh) {
    m_Mesh = InMesh;
}
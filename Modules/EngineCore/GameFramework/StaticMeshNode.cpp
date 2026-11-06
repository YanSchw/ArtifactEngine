#include "StaticMeshNode.h"
#include "Assets/Material.h"
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

Material* StaticMeshNode::GetMaterial(int32_t InSlot) const {
    if (Material* material = GetMaterialOverride(InSlot)) {
        return material;
    }
    return m_Mesh ? m_Mesh->GetMaterial(InSlot) : nullptr;
}

Material* StaticMeshNode::GetMaterialOverride(int32_t InSlot) const {
    return InSlot >= 0 && InSlot < m_MaterialOverrides.Size() ? m_MaterialOverrides[InSlot].Get() : nullptr;
}

void StaticMeshNode::SetMaterialOverride(int32_t InSlot, Material* InMaterial) {
    if (InSlot < 0) {
        return;
    }
    while (m_MaterialOverrides.Size() <= InSlot) {
        m_MaterialOverrides.Add(WeakObjectPtr<Material>());
    }
    m_MaterialOverrides[InSlot] = InMaterial;
    AssetManager::Get().LoadAsset(InMaterial);
}
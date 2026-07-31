#include "SceneRootNode.h"
#include "Assets/Scene.h"

Scene* SceneRootNode::GetScene() const {
    return m_Scene.Get();
}

void SceneRootNode::BindScene(Scene* InScene) {
    m_Scene = InScene;
    m_AssetHandles.Clear();

    if (!InScene) {
        return;
    }

    m_AssetHandles.Add(InScene->GetStreamHandle());
    m_AssetHandles += InScene->GetReferencedAssetHandles();
}

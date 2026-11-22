#include "SceneRootNode.h"
#include "Assets/Scene.h"

Scene* SceneRootNode::GetScene() const {
    return m_Scene.Get();
}

void SceneRootNode::BindScene(Scene* InScene) {
    m_Scene = InScene;
}

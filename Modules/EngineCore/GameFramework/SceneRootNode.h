#pragma once
#include "Node.h"
#include "Assets/Asset.h"
#include "SceneRootNode.gen.h"

class Scene;

/* The root node of a scene */
class SceneRootNode : public Node {
public:
    ARTIFACT_CLASS();

    Scene* GetScene() const;
    void BindScene(Scene* InScene);

private:
    WeakObjectPtr<Scene> m_Scene;
    Array<AssetStreamHandle> m_AssetHandles;
};

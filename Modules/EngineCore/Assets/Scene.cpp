#include "Scene.h"
#include "NodeRecord.h"
#include "AssetManager.h"
#include "GameFramework/World.h"
#include "GameFramework/SceneRootNode.h"
#include "Core/Log.h"

SceneRootNode* Scene::Populate(World& OutWorld) {
    AssetManager::Get().LoadAsset(this);

    SceneRootNode* root = OutWorld.Spawn<SceneRootNode>();
    root->SetName(GetDisplayName());
    if (NodeRecord* record = GetRoot()) {
        record->Apply(*root);
    }
    root->BindScene(this);
    return root;
}

Scene* Scene::CreateEmpty(const String& InDirectory, const String& InName) {
    Scene* scene = Cast<Scene>(AssetManager::Get().CreateAsset(StaticClass(), InDirectory, InName));
    if (!scene) {
        return nullptr;
    }

    NodeRecord* record = new NodeRecord();
    record->ClassName = SceneRootNode::StaticClass().Name;
    scene->SetRoot(SharedObjectPtr<NodeRecord>(record));

    AssetManager::Get().SaveAsset(scene);
    return scene;
}

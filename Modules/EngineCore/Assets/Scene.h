#pragma once
#include "NodeAsset.h"
#include "Scene.gen.h"

class World;
class SceneRootNode;

class Scene : public NodeAsset {
public:
    ARTIFACT_CLASS();
    virtual ~Scene() = default;

    SceneRootNode* Populate(World& OutWorld);

    static Scene* CreateEmpty(const String& InDirectory, const String& InName);
};

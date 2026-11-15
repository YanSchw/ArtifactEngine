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

    virtual String SerializeToJson() const override;
    virtual void DeserializeFromJson(const String& InJson) override;
    virtual bool SaveExternalFiles(const String& InAssetPath) const override;

    static Scene* CreateEmpty(const String& InDirectory, const String& InName);

protected:
    PROPERTY(EditorOnly)
    bool m_OneFilePerNode = false;
};

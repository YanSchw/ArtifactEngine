#pragma once
#include "MajorTab.h"
#include "Object/Pointer.h"
#include "Common/UUID.h"
#include "SceneEditorTab.gen.h"

class Scene;
class SceneRootNode;
class Node;

/** The scene editor: viewport in the center, outliner and details on the left. */
class SceneEditorTab : public MajorTab {
public:
    ARTIFACT_CLASS();

    SceneEditorTab();

    void OpenScene(Scene* InScene);
    Scene* GetScene() const { return m_Scene.Get(); }
    void Save();

    virtual String GetTabTitle() const override;
    virtual VectorImage* GetTabIcon() const override;
    virtual void BuildToolBar(UINode& InToolBar) override;
    virtual Asset* GetEditedAsset() const override;
    virtual Node* GetAssetRootNode() const override;
    virtual void OnAssetSaved(Asset* InAsset) override;

private:
    void BuildLayout();
    void RebuildFromCurrentState();
    void DestroyRoot();
    static bool UsesBlueprint(Node& InNode, const UUID& InBlueprintId);

    WeakObjectPtr<Scene> m_Scene;
    WeakObjectPtr<SceneRootNode> m_Root;
};

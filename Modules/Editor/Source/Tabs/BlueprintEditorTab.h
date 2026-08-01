#pragma once
#include "MajorTab.h"
#include "Object/Pointer.h"
#include "BlueprintEditorTab.gen.h"

class Blueprint;
class Node;
class NodeRecord;
class ViewportTab;

/** Edits one Blueprint: its instance lives alone in a throwaway world. */
class BlueprintEditorTab : public MajorTab {
public:
    ARTIFACT_CLASS();

    BlueprintEditorTab();

    void OpenBlueprint(Blueprint* InBlueprint);
    Blueprint* GetBlueprint() const { return m_Blueprint.Get(); }
    void Save();

    Class GetParentClass() const;
    void SetParentClass(const Class& InClass);
    Array<Class> GetSelectableParentClasses() const;

    virtual String GetTabTitle() const override;
    virtual VectorImage* GetTabIcon() const override;
    virtual void BuildToolBar(UINode& InToolBar) override;
    virtual Asset* GetEditedAsset() const override;
    virtual Node* GetAssetRootNode() const override;
    virtual void OnAssetSaved(Asset* InAsset) override;

private:
    void DestroyInstance();
    void BuildInstanceFrom(NodeRecord& InRecord);
    void SyncViewportMode();

    WeakObjectPtr<Blueprint> m_Blueprint;
    WeakObjectPtr<Node> m_Instance;
    ViewportTab* m_Viewport = nullptr;
    int8_t m_RootWasUI = -1;
};

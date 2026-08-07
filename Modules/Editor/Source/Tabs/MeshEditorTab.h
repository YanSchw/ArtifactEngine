#pragma once
#include "MajorTab.h"
#include "Object/Pointer.h"
#include "MeshEditorTab.gen.h"

class Mesh;
class AssetPreviewTab;
class VectorImage;

class MeshEditorTab : public MajorTab {
public:
    ARTIFACT_CLASS();

    MeshEditorTab();

    void OpenMesh(Mesh* InMesh);
    Mesh* GetMesh() const { return m_Mesh.Get(); }

    void Save();

    virtual String GetTabTitle() const override;
    virtual VectorImage* GetTabIcon() const override;
    virtual void BuildToolBar(UINode& InToolBar) override;
    virtual Asset* GetEditedAsset() const override;
    virtual void OnObjectEdited(Object* InObject) override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;

private:
    void Reimport();

    WeakObjectPtr<Mesh> m_Mesh;
    AssetPreviewTab* m_Preview = nullptr;
    bool m_ReimportPending = false;
};

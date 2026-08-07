#pragma once
#include "MajorTab.h"
#include "Object/Pointer.h"
#include "MaterialEditorTab.gen.h"

class Material;
class AssetPreviewTab;
class DetailsTab;
class VectorImage;

class MaterialEditorTab : public MajorTab {
public:
    ARTIFACT_CLASS();

    MaterialEditorTab();

    void OpenMaterial(Material* InMaterial);
    Material* GetMaterial() const { return m_Material.Get(); }

    void Save();

    virtual String GetTabTitle() const override;
    virtual VectorImage* GetTabIcon() const override;
    virtual void BuildToolBar(UINode& InToolBar) override;
    virtual Asset* GetEditedAsset() const override;
    virtual void OnObjectEdited(Object* InObject) override;
    virtual void OnAssetSaved(Asset* InAsset) override;

private:
    WeakObjectPtr<Material> m_Material;
    AssetPreviewTab* m_Preview = nullptr;
    DetailsTab* m_Details = nullptr;
};

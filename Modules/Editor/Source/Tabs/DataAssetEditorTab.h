#pragma once
#include "MajorTab.h"
#include "Object/Pointer.h"
#include "DataAssetEditorTab.gen.h"

class DataAsset;
class VectorImage;

class DataAssetEditorTab : public MajorTab {
public:
    ARTIFACT_CLASS();

    DataAssetEditorTab();

    void OpenDataAsset(DataAsset* InAsset);
    DataAsset* GetDataAsset() const { return m_Asset.Get(); }

    void Save();

    virtual String GetTabTitle() const override;
    virtual VectorImage* GetTabIcon() const override;
    virtual void BuildToolBar(UINode& InToolBar) override;
    virtual Asset* GetEditedAsset() const override;

private:
    WeakObjectPtr<DataAsset> m_Asset;
};

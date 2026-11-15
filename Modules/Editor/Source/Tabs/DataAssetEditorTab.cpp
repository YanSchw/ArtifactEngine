#include "DataAssetEditorTab.h"

#include "DetailsTab.h"
#include "EditorWindow.h"
#include "UI/UIDockArea.h"
#include "UI/EditorIcons.h"
#include "UI/EditorStyle.h"
#include "Assets/AssetManager.h"
#include "Assets/DataAsset.h"

DataAssetEditorTab::DataAssetEditorTab() {
    GetDockArea()->DockNew<DetailsTab>(UIDockSlot::Center);
}

void DataAssetEditorTab::OpenDataAsset(DataAsset* InAsset) {
    m_Asset = InAsset;

    AssetManager::Get().LoadAsset(InAsset);
    SetSelection(InAsset);

    if (EditorWindow* window = GetOwnerWindow()) {
        window->MarkChromeDirty();
    }
}

void DataAssetEditorTab::Save() {
    DataAsset* asset = m_Asset.Get();
    if (!asset) {
        AE_WARN("There is no data asset open to save");
        return;
    }
    if (asset->GetClass() == DataAsset::StaticClass()) {
        AE_ERROR("'{0}' lost its class (did a script fail to compile?) and is not saved, so its data is kept", asset->GetDisplayName());
        return;
    }
    if (AssetManager::Get().SaveAsset(asset)) {
        BroadcastAssetSaved(asset, this);
    }
}

Asset* DataAssetEditorTab::GetEditedAsset() const {
    return m_Asset.Get();
}

String DataAssetEditorTab::GetTabTitle() const {
    DataAsset* asset = m_Asset.Get();
    return asset ? asset->GetDisplayName() : String("Untitled Data Asset");
}

VectorImage* DataAssetEditorTab::GetTabIcon() const {
    return EditorIcons::DataAsset();
}

void DataAssetEditorTab::BuildToolBar(UINode& InToolBar) {
    EditorStyle::ToolButton(InToolBar, EditorIcons::Save(), "Save", [this] { Save(); });
}

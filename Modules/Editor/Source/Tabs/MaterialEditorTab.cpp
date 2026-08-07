#include "MaterialEditorTab.h"

#include "AssetPreviewTab.h"
#include "DetailsTab.h"
#include "EditorWindow.h"
#include "UI/UIDockArea.h"
#include "UI/EditorIcons.h"
#include "UI/EditorStyle.h"
#include "GameFramework/UIBuilder.h"
#include "Assets/AssetManager.h"
#include "Assets/Material.h"

MaterialEditorTab::MaterialEditorTab() {
    UIDockArea* area = GetDockArea();
    m_Preview = area->DockNew<AssetPreviewTab>(UIDockSlot::Center);
    m_Details = area->DockNew<DetailsTab>(UIDockSlot::Right, nullptr, 0.35f);
}

void MaterialEditorTab::OpenMaterial(Material* InMaterial) {
    m_Material = InMaterial;

    if (InMaterial) {
        AssetManager::Get().LoadAsset(InMaterial);
        InMaterial->RefreshPropertyBuffer();
    }
    m_Preview->SetMaterial(InMaterial);
    SetSelection(InMaterial);

    if (EditorWindow* window = GetOwnerWindow()) {
        window->MarkChromeDirty();
    }
}

void MaterialEditorTab::Save() {
    Material* material = m_Material.Get();
    if (!material) {
        AE_WARN("There is no material open to save");
        return;
    }
    if (AssetManager::Get().SaveAsset(material)) {
        BroadcastAssetSaved(material, this);
    }
}

void MaterialEditorTab::OnObjectEdited(Object* InObject) {
    Material* material = m_Material.Get();
    if (!material || InObject != material) {
        return;
    }
    material->RefreshPropertyBuffer();
    m_Preview->InvalidatePipeline();
}

void MaterialEditorTab::OnAssetSaved(Asset* InAsset) {
    Material* material = m_Material.Get();
    if (!material || InAsset != (Asset*)material->GetBaseGraph()) {
        return;
    }
    // Wiring a pin in the graph retires the input it fed, so the parameter rows are rebuilt.
    material->RefreshPropertyBuffer();
    m_Details->MarkDirty();
    m_Preview->InvalidatePipeline();
}

Asset* MaterialEditorTab::GetEditedAsset() const {
    return m_Material.Get();
}

String MaterialEditorTab::GetTabTitle() const {
    Material* material = m_Material.Get();
    return material ? material->GetDisplayName() : String("Untitled Material");
}

VectorImage* MaterialEditorTab::GetTabIcon() const {
    return EditorIcons::Material();
}

void MaterialEditorTab::BuildToolBar(UINode& InToolBar) {
    UIButton& save = UI::Button(InToolBar, "Save", [this] { Save(); });
    save.Size = { 70.0_px, 1.0_rel };
    EditorStyle::ApplyButtonStyle(save);
}

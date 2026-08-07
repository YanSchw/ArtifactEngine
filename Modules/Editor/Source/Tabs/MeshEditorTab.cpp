#include "MeshEditorTab.h"

#include "AssetPreviewTab.h"
#include "DetailsTab.h"
#include "EditorWindow.h"
#include "UI/UIDockArea.h"
#include "UI/EditorIcons.h"
#include "UI/EditorStyle.h"
#include "GameFramework/UIBuilder.h"
#include "Assets/AssetManager.h"
#include "Assets/Material.h"
#include "Assets/Mesh.h"

MeshEditorTab::MeshEditorTab() {
    UIDockArea* area = GetDockArea();
    m_Preview = area->DockNew<AssetPreviewTab>(UIDockSlot::Center);
    area->DockNew<DetailsTab>(UIDockSlot::Right, nullptr, 0.35f);
}

void MeshEditorTab::OpenMesh(Mesh* InMesh) {
    m_Mesh = InMesh;

    AssetManager::Get().LoadAsset(InMesh);
    m_Preview->SetMesh(InMesh);
    SetSelection(InMesh);

    if (EditorWindow* window = GetOwnerWindow()) {
        window->MarkChromeDirty();
    }
}

void MeshEditorTab::Save() {
    Mesh* mesh = m_Mesh.Get();
    if (!mesh) {
        AE_WARN("There is no mesh open to save");
        return;
    }
    if (AssetManager::Get().SaveAsset(mesh)) {
        BroadcastAssetSaved(mesh, this);
    }
}

void MeshEditorTab::Reimport() {
    m_ReimportPending = false;

    Mesh* mesh = m_Mesh.Get();
    if (!mesh) {
        return;
    }
    mesh->Reimport();
    AssetManager::Get().LoadAsset(mesh->GetMaterial());
    m_Preview->InvalidatePipeline();
}

void MeshEditorTab::OnObjectEdited(Object* InObject) {
    if (InObject == (Object*)m_Mesh.Get()) {
        m_ReimportPending = true;
    }
}

void MeshEditorTab::OnUIUpdate(const UIFrameContext& InContext) {
    if (m_ReimportPending) {
        Reimport();
    }
    MajorTab::OnUIUpdate(InContext);
}

Asset* MeshEditorTab::GetEditedAsset() const {
    return m_Mesh.Get();
}

String MeshEditorTab::GetTabTitle() const {
    Mesh* mesh = m_Mesh.Get();
    return mesh ? mesh->GetDisplayName() : String("Untitled Mesh");
}

VectorImage* MeshEditorTab::GetTabIcon() const {
    return EditorIcons::Mesh();
}

void MeshEditorTab::BuildToolBar(UINode& InToolBar) {
    UIButton& save = UI::Button(InToolBar, "Save", [this] { Save(); });
    save.Size = { 70.0_px, 1.0_rel };
    EditorStyle::ApplyButtonStyle(save);

    UIButton& reimport = UI::Button(InToolBar, "Reimport", [this] { m_ReimportPending = true; });
    reimport.Size = { 90.0_px, 1.0_rel };
    EditorStyle::ApplyButtonStyle(reimport);
}

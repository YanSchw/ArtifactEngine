#include "SceneEditorTab.h"
#include "OutlinerTab.h"
#include "DetailsTab.h"
#include "ViewportTab.h"
#include "UI/UIDockArea.h"
#include "UI/EditorStyle.h"
#include "UI/EditorIcons.h"
#include "GameFramework/UIBuilder.h"
#include "GameFramework/Node3D.h"
#include "GameFramework/SceneRootNode.h"
#include "Assets/AssetManager.h"
#include "Assets/Scene.h"
#include "Assets/Blueprint.h"
#include "Assets/NodeRecord.h"
#include "Core/EngineConfig.h"
#include "Core/Log.h"

SceneEditorTab::SceneEditorTab() {
    SetEditedWorld(new World());
    BuildLayout();
}

void SceneEditorTab::BuildLayout() {
    UIDockArea* area = GetDockArea();
    area->DockNew<ViewportTab>(UIDockSlot::Center);
    OutlinerTab* outliner = area->DockNew<OutlinerTab>(UIDockSlot::Left, nullptr, 0.24f);
    area->DockNew<DetailsTab>(UIDockSlot::Bottom, outliner->GetDockNode(), 0.45f);
}

bool SceneEditorTab::UsesBlueprint(Node& InNode, const UUID& InBlueprintId) {
    const UUID nodeBlueprint = InNode.GetBlueprintId();
    if (nodeBlueprint.IsValid()) {
        if (nodeBlueprint == InBlueprintId) {
            return true;
        }
        Blueprint* used = AssetManager::Get().GetAsset<Blueprint>(nodeBlueprint);
        if (used && used->DependsOn(InBlueprintId)) {
            return true;
        }
    }

    for (uint32_t i = 0; i < InNode.GetChildCount(); i++) {
        if (UsesBlueprint(*InNode.GetChild((int)i), InBlueprintId)) {
            return true;
        }
    }
    return false;
}

void SceneEditorTab::DestroyRoot() {
    ClearSelection();
    if (SceneRootNode* previous = m_Root.Get()) {
        previous->Destroy();
        GetEditedWorld()->ResolvePendingKills();
    }
    m_Root = nullptr;
}

void SceneEditorTab::OpenScene(Scene* InScene) {
    DestroyRoot();
    m_Scene = InScene;
    m_Root = InScene ? GetEditedWorld()->Populate(InScene) : nullptr;
    if (SceneRootNode* root = m_Root.Get()) {
        root->SetName(InScene->GetDisplayName());
    }
}

void SceneEditorTab::RebuildFromCurrentState() {
    Scene* scene = m_Scene.Get();
    SceneRootNode* root = m_Root.Get();
    if (!scene || !root) {
        return;
    }

    SharedObjectPtr<NodeRecord> state = NodeRecord::Capture(*root);
    DestroyRoot();

    SceneRootNode* rebuilt = GetEditedWorld()->Spawn<SceneRootNode>();
    rebuilt->SetName(scene->GetDisplayName());
    state->Apply(*rebuilt);
    rebuilt->BindScene(scene);
    m_Root = rebuilt;
}

void SceneEditorTab::Save() {
    Scene* scene = m_Scene.Get();
    SceneRootNode* root = m_Root.Get();
    if (!scene || !root) {
        AE_WARN("There is no scene open to save");
        return;
    }

    scene->CaptureFrom(*root);
    if (AssetManager::Get().SaveAsset(scene)) {
        BroadcastAssetSaved(scene, this);
    }
}

Asset* SceneEditorTab::GetEditedAsset() const {
    return m_Scene.Get();
}

Node* SceneEditorTab::GetAssetRootNode() const {
    return m_Root.Get();
}

void SceneEditorTab::OnAssetSaved(Asset* InAsset) {
    Scene* scene = m_Scene.Get();
    SceneRootNode* root = m_Root.Get();
    if (!scene || !root) {
        return;
    }

    if (InAsset == scene) {
        OpenScene(scene);
        return;
    }

    if (Blueprint* blueprint = Cast<Blueprint>(InAsset)) {
        if (UsesBlueprint(*root, blueprint->GetId())) {
            RebuildFromCurrentState();
        }
    }
}

String SceneEditorTab::GetTabTitle() const {
    Scene* scene = m_Scene.Get();
    return scene ? scene->GetDisplayName() : String("Untitled Scene");
}

VectorImage* SceneEditorTab::GetTabIcon() const {
    return EditorIcons::Level();
}

void SceneEditorTab::BuildToolBar(UINode& InToolBar) {
    const auto addButton = [&InToolBar](const String& InCaption, std::function<void()> InAction) {
        UIButton& button = UI::Button(InToolBar, InCaption, std::move(InAction));
        button.Size = { 70.0_px, 1.0_rel };
        EditorStyle::ApplyButtonStyle(button);
    };
    addButton("Save", [this] { Save(); });
    addButton("Settings", [] { AE_INFO("SceneEditorTab: Settings"); });
    addButton("Play", [] { AE_INFO("SceneEditorTab: Play"); });
}

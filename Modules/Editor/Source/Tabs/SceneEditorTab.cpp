#include "SceneEditorTab.h"
#include "OutlinerTab.h"
#include "DetailsTab.h"
#include "ViewportTab.h"
#include "EditorEngine.h"
#include "UI/UIDockArea.h"
#include "UI/EditorStyle.h"
#include "UI/EditorIcons.h"
#include "GameFramework/UIBuilder.h"
#include "GameFramework/UIHStack.h"
#include "GameFramework/UIQuad.h"
#include "GameFramework/UISvg.h"
#include "GameFramework/Node3D.h"
#include "GameFramework/SceneRootNode.h"
#include "Assets/AssetManager.h"
#include "Assets/Scene.h"
#include "Assets/Blueprint.h"
#include "Assets/NodeRecord.h"
#include "GameFramework/GameInstance.h"
#include "EditorWindow.h"
#include "Core/EngineConfig.h"
#include "Core/Log.h"

SceneEditorTab::SceneEditorTab() {
    SetEditedWorld(new World());
    BuildLayout();
}

SceneEditorTab::~SceneEditorTab() {
    StopPlayInEditor();
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
        GetAuthoringWorld()->ResolvePendingKills();
    }
    m_Root = nullptr;
}

void SceneEditorTab::OpenScene(Scene* InScene) {
    StopPlayInEditor();
    DestroyRoot();
    m_Scene = InScene;
    m_Root = InScene ? GetAuthoringWorld()->Populate(InScene) : nullptr;
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

    SceneRootNode* rebuilt = GetAuthoringWorld()->Spawn<SceneRootNode>();
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

Node* SceneEditorTab::GetAuthoringRootNode() const {
    return m_Root.Get();
}

void SceneEditorTab::PlayInEditor(PlayState InState) {
    Node* source = m_Root.Get();
    if (!source) {
        AE_WARN("Play In Editor needs an open scene");
        return;
    }

    EditorEngine* engine = EditorEngine::Get();
    if (!engine) {
        return;
    }

    StopPlayInEditor();
    ClearSelection();

    GameInstance* game = engine->CreatePlayInstance(this);
    World* world = game->CreateNewWorld(true);

    const SharedObjectPtr<NodeRecord> state = NodeRecord::Capture(*source);
    SceneRootNode* root = world->Spawn<SceneRootNode>();
    root->SetName(source->GetName());
    state->Apply(*root);
    if (Scene* scene = m_Scene.Get()) {
        root->BindScene(scene);
    }

    m_PlayWorld = world;
    m_PlayRoot = root;
    m_PlayState = InState;
    m_CursorWasLocked = false;
    EditorWindow::MarkAllChromeDirty();
}

void SceneEditorTab::StopPlayInEditor() {
    if (m_PlayState == PlayState::Editor) {
        return;
    }
    m_PlayState = PlayState::Editor;
    m_PlayWorld = nullptr;
    m_PlayRoot = nullptr;
    ClearSelection();

    if (EditorWindow* window = GetOwnerWindow()) {
        window->SetCursorLocked(false);
    }
    if (EditorEngine* engine = EditorEngine::Get()) {
        engine->DisposePlayInstance();
    }
    EditorWindow::MarkAllChromeDirty();
}

void SceneEditorTab::SetPlayState(PlayState InState) {
    if (m_PlayState == PlayState::Editor || m_PlayState == InState) {
        return;
    }

    EditorWindow* window = GetOwnerWindow();
    if (window && InState == PlayState::Simulating) {
        m_CursorWasLocked = window->IsCursorLocked();
        window->SetCursorLocked(false);
    } else if (window && m_CursorWasLocked) {
        window->SetCursorLocked(true);
    }

    m_PlayState = InState;
    EditorWindow::MarkAllChromeDirty();
}

Asset* SceneEditorTab::GetEditedAsset() const {
    return m_Scene.Get();
}

World* SceneEditorTab::GetEditedWorld() const {
    if (World* playWorld = m_PlayWorld.Get()) {
        return playWorld;
    }
    return GetAuthoringWorld();
}

Node* SceneEditorTab::GetAssetRootNode() const {
    if (m_PlayWorld.Get()) {
        if (Node* playRoot = m_PlayRoot.Get()) {
            return playRoot;
        }
    }
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

void SceneEditorTab::BuildPlayControls(UINode& InToolBar) {
    UIQuad* divider = InToolBar.Add<UIQuad>();
    divider->Size = { 1.0_px, 1.0_rel };
    divider->Color = EditorStyle::Border;

    if (m_PlayState == PlayState::Editor) {
        EditorStyle::IconButton(InToolBar, EditorIcons::Play(), EditorStyle::TransformY, "Play", 74.0f,
                                [this] { PlayInEditor(PlayState::Playing); });
        EditorStyle::IconButton(InToolBar, EditorIcons::Simulate(), EditorStyle::Text, "Simulate", 92.0f,
                                [this] { PlayInEditor(PlayState::Simulating); });
        return;
    }

    EditorStyle::IconButton(InToolBar, EditorIcons::Stop(), EditorStyle::TransformX, "Stop", 72.0f,
                            [this] { StopPlayInEditor(); });

    const bool playing = (m_PlayState == PlayState::Playing);
    EditorStyle::IconButton(InToolBar, playing ? EditorIcons::Eject() : EditorIcons::Possess(),
                            EditorStyle::AccentBright, playing ? "Eject" : "Possess", 88.0f,
                            [this, playing] { SetPlayState(playing ? PlayState::Simulating : PlayState::Playing); });

    UILabel* status = InToolBar.Add<UILabel>();
    status->Size = { 96.0_px, 1.0_rel };
    status->FontSize = EditorStyle::FontSize;
    status->Color = EditorStyle::AccentBright;
    status->VAlign = UIVAlign::Middle;
    status->Padding = UIPadding(8.0f, 0.0f, 0.0f, 0.0f);
    status->Bind = [this, status] {
        status->Text = m_PlayState == PlayState::Playing ? "Playing" : "Simulating";
    };
}

void SceneEditorTab::BuildToolBar(UINode& InToolBar) {
    const auto addButton = [&InToolBar](const String& InCaption, std::function<void()> InAction) {
        UIButton& button = UI::Button(InToolBar, InCaption, std::move(InAction));
        button.Size = { 70.0_px, 1.0_rel };
        EditorStyle::ApplyButtonStyle(button);
    };
    addButton("Save", [this] { Save(); });
    addButton("Settings", [] { AE_INFO("SceneEditorTab: Settings"); });

    BuildPlayControls(InToolBar);
}

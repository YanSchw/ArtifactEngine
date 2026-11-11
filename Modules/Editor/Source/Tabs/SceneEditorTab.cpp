#include "SceneEditorTab.h"
#include "EventGraphTab.h"
#include "UI/UIDockNode.h"
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
#include "GameFramework/UILabel.h"
#include "UI/UIRoundedQuad.h"
#include "Assets/Font.h"
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
    ViewportTab* viewport = area->DockNew<ViewportTab>(UIDockSlot::Center);
    area->DockNew<EventGraphTab>(UIDockSlot::Center, viewport->GetDockNode());
    viewport->GetDockNode()->SetActiveTab(viewport);
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

    if (EditorEngine* engine = EditorEngine::Get()) {
        engine->DisposePlayInstance();
    }
    EditorWindow::MarkAllChromeDirty();
}

void SceneEditorTab::SetPlayState(PlayState InState) {
    if (m_PlayState == PlayState::Editor || m_PlayState == InState) {
        return;
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

static constexpr float s_PlaySegmentWidth = 38.0f;
static constexpr float s_PlayStatusInset = 9.0f;
static constexpr float s_PlayStatusDot = 7.0f;
static constexpr float s_PlayStatusGap = 6.0f;

static void AddPlaySegment(UINode& InGroup, VectorImage* InIcon, const Vec4& InTint, const String& InTooltip,
                           bool InFirst, bool InLast, std::function<void()> InAction) {
    UIButton& button = EditorStyle::ToolButton(InGroup, InIcon, InTooltip, std::move(InAction), s_PlaySegmentWidth);
    EditorStyle::StyleSegment(button, InFirst, InLast);
    button.HoverColor = Vec4(glm::mix(Vec3(EditorStyle::ButtonHover), Vec3(InTint), 0.2f), 1.0f);
    if (UISvg* icon = button.GetChildByClass(UISvg::StaticClass())->As<UISvg>()) {
        icon->Tint = InTint;
    }
}

static void AddPlayStatus(UINode& InToolBar, const String& InText, const Vec4& InColor) {
    Font* font = UINode::GetDefaultFont();
    const float textWidth = font ? font->GetTextWidth(InText, EditorStyle::FontSize) : 7.0f * (float)InText.size();
    const float textLeft = s_PlayStatusInset + s_PlayStatusDot + s_PlayStatusGap;

    UINode* slot = InToolBar.Add<UINode>();
    slot->Size = { UIValue(textLeft + textWidth + s_PlayStatusInset), 1.0_rel };

    UIRoundedQuad* pill = slot->Add<UIRoundedQuad>();
    pill->Center({ 1.0_rel, 22.0_px });
    pill->Color = Vec4(Vec3(InColor), 0.14f);
    pill->CornerRadius = Vec4(11.0f);

    UIRoundedQuad* dot = pill->Add<UIRoundedQuad>();
    dot->Anchor = dot->Pivot = Vec2(0.0f, 0.5f);
    dot->Position = Vec2(s_PlayStatusInset, 0.0f);
    dot->Size = Vec2(s_PlayStatusDot);
    dot->Color = InColor;
    dot->CornerRadius = Vec4(s_PlayStatusDot * 0.5f);

    UILabel* label = pill->Add<UILabel>();
    label->Anchor = label->Pivot = Vec2(0.0f, 0.5f);
    label->Position = Vec2(textLeft, 0.0f);
    label->Size = { UIValue(textWidth + 2.0f), 1.0_rel };
    label->FontSize = EditorStyle::FontSize;
    label->Color = InColor;
    label->VAlign = UIVAlign::Middle;
    label->Text = InText;
}

void SceneEditorTab::BuildPlayControls(UINode& InToolBar) {
    const bool running = m_PlayState != PlayState::Editor;
    const bool playing = m_PlayState == PlayState::Playing;

    UIQuad* divider = InToolBar.Add<UIQuad>();
    divider->Size = { 1.0_px, 1.0_rel };
    divider->Color = EditorStyle::Border;

    UIHStack* group = InToolBar.Add<UIHStack>();
    group->Size = { UIValue(s_PlaySegmentWidth * 2.0f), 1.0_rel };

    if (!running) {
        AddPlaySegment(*group, EditorIcons::Play(), EditorStyle::TransformY, "Play (F5)", true, false,
                       [this] { PlayInEditor(PlayState::Playing); });
        AddPlaySegment(*group, EditorIcons::Simulate(), EditorStyle::Text, "Simulate (F6)", false, true,
                       [this] { PlayInEditor(PlayState::Simulating); });
        return;
    }

    AddPlaySegment(*group, EditorIcons::Stop(), EditorStyle::TransformX, "Stop (F5)", true, false,
                   [this] { StopPlayInEditor(); });
    AddPlaySegment(*group, playing ? EditorIcons::Eject() : EditorIcons::Possess(), EditorStyle::AccentBright,
                   playing ? "Eject (F7)" : "Possess (F7)", false, true,
                   [this, playing] { SetPlayState(playing ? PlayState::Simulating : PlayState::Playing); });
    AddPlayStatus(InToolBar, playing ? "Playing" : "Simulating", playing ? EditorStyle::TransformY : EditorStyle::AccentBright);
}

void SceneEditorTab::BuildToolBar(UINode& InToolBar) {
    EditorStyle::ToolButton(InToolBar, EditorIcons::Save(), "Save", [this] { Save(); });

    BuildPlayControls(InToolBar);
}

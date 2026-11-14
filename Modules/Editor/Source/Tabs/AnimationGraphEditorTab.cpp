#include "AnimationGraphEditorTab.h"
#include "DetailsTab.h"
#include "EditorWindow.h"
#include "Graph/AnimationGraphView.h"
#include "UI/UIDockArea.h"
#include "UI/EditorIcons.h"
#include "UI/EditorStyle.h"
#include "UI/UICheckbox.h"
#include "UI/UIContextMenu.h"
#include "UI/UIDragNumber.h"
#include "UI/UIMenuModel.h"
#include "GameFramework/UIButton.h"
#include "GameFramework/UIHStack.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UIQuad.h"
#include "GameFramework/UIScrollArea.h"
#include "GameFramework/UISvg.h"
#include "GameFramework/UITextArea.h"
#include "GameFramework/UIVStack.h"
#include "GameFramework/AnimatorNode.h"
#include "Assets/AnimationGraph.h"
#include "Assets/AssetManager.h"

static constexpr float s_ParameterRowHeight = 26.0f;
static constexpr float s_LiveSearchInterval = 0.5f;

AnimationGraphEditorTab::AnimationGraphEditorTab() {
    UIDockArea* area = GetDockArea();
    AnimationGraphCanvasTab* canvas = area->DockNew<AnimationGraphCanvasTab>(UIDockSlot::Center);
    area->DockNew<AnimationParametersTab>(UIDockSlot::Left, canvas->GetDockNode(), 0.2f);
    area->DockNew<DetailsTab>(UIDockSlot::Right, canvas->GetDockNode(), 0.28f);
}

void AnimationGraphEditorTab::OpenGraph(AnimationGraph* InGraph) {
    ClearSelection();
    m_Graph = InGraph;
    m_LiveAnimator = nullptr;
    if (InGraph) {
        AssetManager::Get().LoadAsset(InGraph);
    }
    if (EditorWindow* window = GetOwnerWindow()) {
        window->MarkChromeDirty();
    }
}

void AnimationGraphEditorTab::Save() {
    AnimationGraph* graph = m_Graph.Get();
    if (!graph) {
        AE_WARN("There is no animation graph open to save");
        return;
    }
    if (AssetManager::Get().SaveAsset(graph)) {
        BroadcastAssetSaved(graph, this);
    }
}

AnimatorNode* AnimationGraphEditorTab::FindLiveAnimator() const {
    AnimationGraph* graph = m_Graph.Get();
    if (!graph) {
        return nullptr;
    }
    const auto runs = [graph](Object* InObject) {
        AnimatorNode* animator = Cast<AnimatorNode>(InObject);
        return animator && animator->IsPlaying() && animator->GetAnimationGraph() == graph ? animator : nullptr;
    };

    AnimatorNode* found = nullptr;
    for (const SharedObjectPtr<ThemedWindow>& window : ThemedWindow::GetAllWindows()) {
        EditorWindow* editorWindow = window.Get() ? window->As<EditorWindow>() : nullptr;
        for (MajorTab* tab : editorWindow ? editorWindow->GetOpenTabs() : Array<MajorTab*>()) {
            World* world = tab->GetPlayState() != PlayState::Editor ? tab->GetEditedWorld() : nullptr;
            if (!world) {
                continue;
            }
            if (AnimatorNode* selected = runs(tab->GetSoleSelection())) {
                return selected;
            }
            for (Node* node : world->GetAllNodes()) {
                found = found ? found : runs(node);
            }
        }
    }
    return found;
}

void AnimationGraphEditorTab::OnUIUpdate(const UIFrameContext& InContext) {
    MajorTab::OnUIUpdate(InContext);

    AnimatorNode* live = m_LiveAnimator.Get();
    if (live && (!live->IsPlaying() || live->GetAnimationGraph() != m_Graph.Get())) {
        m_LiveAnimator = nullptr;
    }
    m_LiveSearchCooldown -= InContext.DeltaTime;
    if (m_LiveSearchCooldown <= 0.0f) {
        m_LiveSearchCooldown = s_LiveSearchInterval;
        m_LiveAnimator = FindLiveAnimator();
    }
}

Asset* AnimationGraphEditorTab::GetEditedAsset() const {
    return m_Graph.Get();
}

String AnimationGraphEditorTab::GetTabTitle() const {
    AnimationGraph* graph = m_Graph.Get();
    return graph ? graph->GetDisplayName() : String("Untitled Animation Graph");
}

VectorImage* AnimationGraphEditorTab::GetTabIcon() const {
    return EditorIcons::AnimationGraph();
}

void AnimationGraphEditorTab::BuildToolBar(UINode& InToolBar) {
    EditorStyle::ToolButton(InToolBar, EditorIcons::Save(), "Save", [this] { Save(); });

    UILabel* live = InToolBar.Add<UILabel>();
    live->Size = { 1.0_rel, 1.0_rel };
    live->Padding = UIPadding(10.0f, 0.0f);
    live->FontSize = EditorStyle::FontSize;
    live->VAlign = UIVAlign::Middle;
    live->Color = EditorStyle::AccentBright;
    live->Bind = [this, live] {
        AnimatorNode* animator = GetLiveAnimator();
        live->Text = animator ? "Live: " + animator->GetName() : String();
    };
}

/* ---------------------------------- Canvas ---------------------------------- */

AnimationGraphCanvasTab::AnimationGraphCanvasTab() {
    m_View = Add<AnimationGraphView>();
    m_View->Fill();
    m_View->OnSelectionChanged = [this] {
        MajorTab* owner = GetMajorTab();
        if (!owner) {
            return;
        }
        Array<Object*> selection;
        for (GraphNode* node : m_View->GetSelectedNodes()) {
            selection.Add(node);
        }
        if (GraphConnection* connection = m_View->GetSelectedConnection()) {
            selection.Add(connection);
        }
        owner->SetSelection(selection);
    };
    m_View->OnGraphChanged = [this] {
        AnimationStateMachine* machine = m_View->GetStateMachine();
        if (!machine || machine->GetDefaultState()) {
            return;
        }
        for (const SharedObjectPtr<GraphNode>& node : machine->Nodes) {
            if (AnimationStateNode* state = Cast<AnimationStateNode>(node.Get())) {
                machine->SetDefaultState(*state);
                return;
            }
        }
    };
    m_View->OnSaveRequested = [this] {
        if (AnimationGraphEditorTab* editor = GetEditor()) {
            editor->Save();
        }
    };
}

VectorImage* AnimationGraphCanvasTab::GetTabIcon() const {
    return EditorIcons::AnimationGraph();
}

AnimationGraphEditorTab* AnimationGraphCanvasTab::GetEditor() const {
    return Cast<AnimationGraphEditorTab>(GetMajorTab());
}

void AnimationGraphCanvasTab::OnBind() {
    AnimationGraphEditorTab* editor = GetEditor();
    AnimationGraph* graph = editor ? editor->GetGraph() : nullptr;
    AnimationStateMachine* machine = graph ? graph->GetStateMachine() : nullptr;
    if (m_View->GetGraph() != machine) {
        m_View->SetGraph(SharedObjectPtr<NodeGraph>(machine), m_ShownGraph.Get() != graph);
        m_ShownGraph = graph;
    }
    m_View->LiveAnimator = editor ? editor->GetLiveAnimator() : nullptr;
    MinorTab::OnBind();
}

/* -------------------------------- Parameters -------------------------------- */

AnimationParametersTab::AnimationParametersTab() {
    UIVStack* layout = Add<UIVStack>();
    layout->Fill();

    UIQuad* header = layout->Add<UIQuad>();
    header->Size = { 1.0_rel, 30.0_px };
    header->Color = EditorStyle::ToolBar;

    UILabel* title = header->Add<UILabel>();
    title->Fill();
    title->Padding = UIPadding(10.0f, 0.0f);
    title->FontSize = EditorStyle::FontSize;
    title->Color = EditorStyle::TextDim;
    title->VAlign = UIVAlign::Middle;
    title->Text = "Parameters";

    UIButton* add = header->Add<UIButton>();
    add->Anchor = add->Pivot = Vec2(1.0f, 0.5f);
    add->Position = Vec2(-6.0f, 0.0f);
    add->Size = Vec2(22.0f, 22.0f);
    EditorStyle::StyleButton(*add);
    UISvg* addIcon = add->Add<UISvg>();
    addIcon->Center(Vec2(12.0f, 12.0f));
    addIcon->Image = EditorIcons::Add();
    addIcon->Tint = EditorStyle::Text;
    add->Clicked = [this, add] {
        UIMenuModel menu;
        menu.Section("Add Parameter");
        for (AnimationParameterType type : { AnimationParameterType::Float, AnimationParameterType::Int,
                                             AnimationParameterType::Bool, AnimationParameterType::Trigger }) {
            menu.Item(Enum("AnimationParameterType").ConvertValueToString((int64_t)type), [this, type] {
                Edit("Add Parameter", [type](AnimationGraph& OutGraph) { OutGraph.AddParameter(type); });
            });
        }
        UIContextMenu::OpenUnder(*add, menu);
    };

    UIScrollArea* scroll = layout->Add<UIScrollArea>();
    scroll->Size = { 1.0_rel, 1.0_rel };

    m_Rows = scroll->Add<UIVStack>();
    m_Rows->Anchor = m_Rows->Pivot = Vec2(0.0f);
    m_Rows->Position = Vec2(0.0f);
    m_Rows->Size = { 1.0_rel, 0.0_px };
}

VectorImage* AnimationParametersTab::GetTabIcon() const {
    return EditorIcons::Details();
}

AnimationGraph* AnimationParametersTab::GetGraph() const {
    AnimationGraphEditorTab* editor = Cast<AnimationGraphEditorTab>(GetMajorTab());
    return editor ? editor->GetGraph() : nullptr;
}

void AnimationParametersTab::Edit(const String& InTitle, const std::function<void(AnimationGraph&)>& InEdit) {
    AnimationGraph* graph = GetGraph();
    if (!graph) {
        return;
    }
    RecordEdit(InTitle, graph);
    InEdit(*graph);
}

void AnimationParametersTab::OnBind() {
    AnimationGraph* graph = GetGraph();
    const int32_t count = graph ? graph->GetParameters().Size() : 0;
    if (graph != m_BuiltFor.Get() || count != m_BuiltCount) {
        Rebuild();
    }
    MinorTab::OnBind();
}

void AnimationParametersTab::Rebuild() {
    while (m_Rows->GetChildCount() > 0) {
        delete m_Rows->GetChild(0);
    }
    AnimationGraph* graph = GetGraph();
    m_BuiltFor = graph;
    m_BuiltCount = graph ? graph->GetParameters().Size() : 0;
    for (int32_t index = 0; index < m_BuiltCount; index++) {
        BuildRow(index);
    }
    m_Rows->Size = { 1.0_rel, UIValue((float)m_BuiltCount * s_ParameterRowHeight) };
}

void AnimationParametersTab::BuildRow(int32_t InIndex) {
    const WeakObjectPtr<AnimationParametersTab> self = this;
    const auto parameter = [self, InIndex]() -> const AnimationParameter* {
        AnimationGraph* graph = self.Get() ? self.Get()->GetGraph() : nullptr;
        return graph && InIndex < graph->GetParameters().Size() ? &graph->GetParameters()[InIndex] : nullptr;
    };
    const auto live = [self]() -> AnimatorNode* {
        AnimationGraphEditorTab* editor = self.Get() ? Cast<AnimationGraphEditorTab>(self.Get()->GetMajorTab()) : nullptr;
        return editor ? editor->GetLiveAnimator() : nullptr;
    };
    const auto read = [parameter, live]() -> float {
        const AnimationParameter* current = parameter();
        AnimatorNode* animator = live();
        return !current ? 0.0f : animator ? animator->GetFloat(current->Name) : current->DefaultValue;
    };
    const auto write = [self, parameter, live, InIndex](float InValue) {
        const AnimationParameter* current = parameter();
        if (!current || !self.Get()) {
            return;
        }
        if (AnimatorNode* animator = live()) {
            animator->SetFloat(current->Name, InValue);
            return;
        }
        self.Get()->Edit("Edit Parameter", [InIndex, InValue](AnimationGraph& OutGraph) { OutGraph.SetParameterDefault(InIndex, InValue); });
    };

    UIHStack* row = m_Rows->Add<UIHStack>();
    row->Size = { 1.0_rel, UIValue(s_ParameterRowHeight) };
    row->Padding = UIPadding(6.0f, 2.0f);
    row->Gap = 4.0f;

    UITextArea* name = row->Add<UITextArea>();
    name->Size = { 1.0_rel, 1.0_rel };
    name->SingleLine = true;
    name->FontSize = EditorStyle::FontSize;
    name->TextColor = EditorStyle::Text;
    name->CaretColor = EditorStyle::TextBright;
    name->BackgroundColor = EditorStyle::PanelDark;
    name->FocusedBorderColor = EditorStyle::Accent;
    name->Padding = UIPadding(6.0f, 0.0f);
    name->Bind = [name, parameter] {
        const AnimationParameter* current = parameter();
        if (current && !name->IsFocused()) {
            name->Text = current->Name;
        }
    };
    const auto rename = [self, InIndex](const String& InText) {
        if (self.Get()) {
            self.Get()->Edit("Rename Parameter", [InIndex, InText](AnimationGraph& OutGraph) { OutGraph.RenameParameter(InIndex, InText); });
        }
    };
    name->Submitted = rename;
    name->FocusLost = [name, rename] { rename(name->Text); };

    const AnimationParameter* current = parameter();
    const bool toggles = current && (current->Type == AnimationParameterType::Bool || current->Type == AnimationParameterType::Trigger);
    UINode* value = row->Add<UINode>();
    value->Size = { UIValue(toggles ? 24.0f : 64.0f), 1.0_rel };
    if (toggles) {
        UICheckbox* checkbox = value->Add<UICheckbox>();
        checkbox->Anchor = checkbox->Pivot = Vec2(0.5f);
        checkbox->Bind = [checkbox, read] { checkbox->IsOn = read() != 0.0f; };
        checkbox->Changed = [write](bool InValue) { write(InValue ? 1.0f : 0.0f); };
    } else {
        UIDragNumber* drag = value->Add<UIDragNumber>();
        drag->Fill();
        drag->Integer = current && current->Type == AnimationParameterType::Int;
        drag->Get = [read] { return (double)read(); };
        drag->Set = [write](double InValue) { write((float)InValue); };
    }

    UIButton* remove = row->Add<UIButton>();
    remove->Size = { 20.0_px, 1.0_rel };
    remove->NormalColor = Vec4(0.0f);
    remove->HoverColor = EditorStyle::TabHover;
    UISvg* removeIcon = remove->Add<UISvg>();
    removeIcon->Center(Vec2(12.0f, 12.0f));
    removeIcon->Image = EditorIcons::Delete();
    removeIcon->Tint = EditorStyle::TextDim;
    remove->Clicked = [self, InIndex] {
        if (self.Get()) {
            self.Get()->Edit("Remove Parameter", [InIndex](AnimationGraph& OutGraph) { OutGraph.RemoveParameter(InIndex); });
        }
    };
}

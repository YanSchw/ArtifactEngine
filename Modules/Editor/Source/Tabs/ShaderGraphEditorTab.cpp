#include "ShaderGraphEditorTab.h"

#include "DetailsTab.h"
#include "ShaderGraphCanvasTab.h"
#include "ShaderGraphPreviewTab.h"
#include "UI/UIDockArea.h"
#include "UI/EditorIcons.h"
#include "UI/EditorStyle.h"
#include "GameFramework/UIBuilder.h"
#include "GameFramework/UILabel.h"
#include "UI/UIDropdown.h"
#include "Rendering/ShaderTemplate.h"
#include <memory>
#include "Assets/AssetManager.h"
#include "EditorWindow.h"
#include "Assets/ShaderGraph.h"
#include "Graph/GraphNode.h"

ShaderGraphEditorTab::ShaderGraphEditorTab() {
    UIDockArea* area = GetDockArea();
    m_Canvas = area->DockNew<ShaderGraphCanvasTab>(UIDockSlot::Center);
    m_Preview = area->DockNew<ShaderGraphPreviewTab>(UIDockSlot::Left, nullptr, 0.28f);
    area->DockNew<DetailsTab>(UIDockSlot::Bottom, m_Preview->GetDockNode(), 0.55f);
}

void ShaderGraphEditorTab::OpenShaderGraph(ShaderGraph* InShaderGraph) {
    ClearSelection();
    m_ShaderGraph = InShaderGraph;

    if (InShaderGraph) {
        AssetManager::Get().LoadAsset(InShaderGraph);
    }

    m_Canvas->SetShaderGraph(InShaderGraph);
    m_Preview->SetShaderGraph(InShaderGraph);
    RequestRecompile();

    if (EditorWindow* window = GetOwnerWindow()) {
        window->MarkChromeDirty();
    }
}

void ShaderGraphEditorTab::RequestRecompile() {
    m_RecompilePending = true;
}

void ShaderGraphEditorTab::Recompile() {
    m_RecompilePending = false;

    ShaderGraph* graph = m_ShaderGraph.Get();
    if (!graph) {
        return;
    }

    m_CompileError.clear();
    if (!graph->Recompile(m_CompileError)) {
        AE_WARN("ShaderGraph '{0}' failed to compile:\n{1}", graph->GetDisplayName(), m_CompileError);
    }
    m_Preview->InvalidatePipeline();
    RefreshStatusLabel();
}

void ShaderGraphEditorTab::OnUIUpdate(const UIFrameContext& InContext) {
    if (m_RecompilePending) {
        Recompile();
    }
    MajorTab::OnUIUpdate(InContext);
}

void ShaderGraphEditorTab::Save() {
    ShaderGraph* graph = m_ShaderGraph.Get();
    if (!graph) {
        AE_WARN("There is no shader graph open to save");
        return;
    }
    if (AssetManager::Get().SaveAsset(graph)) {
        BroadcastAssetSaved(graph, this);
    }
}

void ShaderGraphEditorTab::OnObjectEdited(Object* InObject) {
    if (Cast<GraphNode>(InObject)) {
        RequestRecompile();
    }
}

Asset* ShaderGraphEditorTab::GetEditedAsset() const {
    return m_ShaderGraph.Get();
}

String ShaderGraphEditorTab::GetTabTitle() const {
    ShaderGraph* graph = m_ShaderGraph.Get();
    return graph ? graph->GetDisplayName() : String("Untitled Shader Graph");
}

VectorImage* ShaderGraphEditorTab::GetTabIcon() const {
    return EditorIcons::GraphEditor();
}

void ShaderGraphEditorTab::BuildToolBar(UINode& InToolBar) {
    UIButton& save = UI::Button(InToolBar, "Save", [this] { Save(); });
    save.Size = { 70.0_px, 1.0_rel };
    EditorStyle::ApplyButtonStyle(save);

    BuildTemplateDropdown(InToolBar);
    BuildStateDropdowns(InToolBar);

    UILabel* status = InToolBar.Add<UILabel>();
    status->Size = { 1.0_rel, 1.0_rel };
    status->FontSize = EditorStyle::FontSize;
    status->HAlign = UIHAlign::Left;
    status->VAlign = UIVAlign::Middle;
    m_StatusLabel = status;
    RefreshStatusLabel();
}

void ShaderGraphEditorTab::BuildTemplateDropdown(UINode& InToolBar) {
    auto paths = std::make_shared<Array<String>>();

    UIDropdown* dropdown = InToolBar.Add<UIDropdown>();
    dropdown->Size = { 150.0_px, 1.0_rel };
    dropdown->GetOptions = [this, paths] {
        Array<String> labels;
        paths->Clear();
        for (const ShaderTemplateInfo& info : ShaderTemplate::FindAll()) {
            paths->Add(info.Path);
            labels.Add(info.DisplayName);
        }
        return labels;
    };
    dropdown->GetSelectedLabel = [this] {
        ShaderGraph* graph = m_ShaderGraph.Get();
        return graph ? graph->GetTemplate().GetDisplayName() : String();
    };
    dropdown->GetSelectedIndex = [this, paths] {
        ShaderGraph* graph = m_ShaderGraph.Get();
        return graph ? paths->IndexOf(graph->GetTemplatePath()) : -1;
    };
    dropdown->SelectionChanged = [this, paths](int32_t InIndex) {
        ShaderGraph* graph = m_ShaderGraph.Get();
        if (graph && InIndex >= 0 && InIndex < paths->Size()) {
            graph->SetTemplatePath((*paths)[InIndex]);
            RequestRecompile();
            if (EditorWindow* window = GetOwnerWindow()) {
                window->MarkChromeDirty();
            }
        }
    };
}

void ShaderGraphEditorTab::BuildStateDropdowns(UINode& InToolBar) {
    ShaderGraph* graph = m_ShaderGraph.Get();
    if (!graph) {
        return;
    }

    for (const String& state : graph->GetStateNames()) {
        auto options = std::make_shared<Array<String>>(graph->GetStateOptions(state));

        UIDropdown* dropdown = InToolBar.Add<UIDropdown>();
        dropdown->Size = { 110.0_px, 1.0_rel };
        dropdown->GetOptions = [options] { return *options; };
        dropdown->GetSelectedLabel = [this, state] {
            ShaderGraph* current = m_ShaderGraph.Get();
            return current ? current->GetStateValue(state) : String();
        };
        dropdown->GetSelectedIndex = [this, state, options] {
            ShaderGraph* current = m_ShaderGraph.Get();
            return current ? options->IndexOf(current->GetStateValue(state)) : -1;
        };
        dropdown->SelectionChanged = [this, state, options](int32_t InIndex) {
            ShaderGraph* current = m_ShaderGraph.Get();
            if (current && InIndex >= 0 && InIndex < options->Size()) {
                current->SetStateValue(state, (*options)[InIndex]);
                RequestRecompile();
            }
        };
    }
}

void ShaderGraphEditorTab::RefreshStatusLabel() {
    UILabel* status = m_StatusLabel.Get();
    if (!status) {
        return;
    }
    if (m_CompileError.empty()) {
        status->Text = "Compiled";
        status->Color = EditorStyle::TextDim;
        return;
    }
    const size_t lineEnd = m_CompileError.find('\n');
    status->Text = lineEnd == String::npos ? m_CompileError : m_CompileError.substr(0, lineEnd);
    status->Color = HexColor(0xE06C6C);
}

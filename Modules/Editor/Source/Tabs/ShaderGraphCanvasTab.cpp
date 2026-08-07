#include "ShaderGraphCanvasTab.h"

#include "Graph/GraphEditorView.h"
#include "MajorTab.h"
#include "ShaderGraphEditorTab.h"
#include "UI/EditorIcons.h"
#include "Assets/ShaderGraph.h"

ShaderGraphCanvasTab::ShaderGraphCanvasTab() {
    m_View = Add<GraphEditorView>();
    m_View->Fill();

    m_View->OnGraphChanged = [this] {
        if (ShaderGraphEditorTab* owner = Cast<ShaderGraphEditorTab>(GetMajorTab())) {
            owner->RequestRecompile();
        }
    };
    m_View->OnSelectionChanged = [this] {
        MajorTab* owner = GetMajorTab();
        if (!owner) {
            return;
        }
        Array<Object*> selection;
        for (GraphNode* node : m_View->GetSelectedNodes()) {
            selection.Add(node);
        }
        owner->SetSelection(selection);
    };
    m_View->OnSaveRequested = [this] {
        if (ShaderGraphEditorTab* owner = Cast<ShaderGraphEditorTab>(GetMajorTab())) {
            owner->Save();
        }
    };
}

void ShaderGraphCanvasTab::SetShaderGraph(ShaderGraph* InShaderGraph) {
    m_View->SetGraph(InShaderGraph ? InShaderGraph->GetGraph() : nullptr);
}

VectorImage* ShaderGraphCanvasTab::GetTabIcon() const {
    return EditorIcons::GraphEditor();
}

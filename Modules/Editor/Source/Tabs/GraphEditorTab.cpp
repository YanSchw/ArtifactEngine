#include "GraphEditorTab.h"
#include "Graph/GraphEditorView.h"
#include "Graph/GraphExampleNodes.h"

static const String s_DemoGraphFile = "NodeGraphExample.json";

GraphEditorTab::GraphEditorTab() {
    m_Graph = NodeGraph::LoadFromFile(s_DemoGraphFile);
    if (!m_Graph) {
        m_Graph = BuildExampleNodeGraph();
    }

    m_View = Add<GraphEditorView>();
    m_View->Fill();
    m_View->SetGraph(m_Graph);
    m_View->OnSaveRequested = [this] { m_Graph->SaveToFile(s_DemoGraphFile); };
}

String GraphEditorTab::GetTabTitle() const {
    return m_Graph ? m_Graph->GraphName : "Node Graph";
}

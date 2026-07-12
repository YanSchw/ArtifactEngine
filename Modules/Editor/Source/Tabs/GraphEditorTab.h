#pragma once
#include "MinorTab.h"
#include "Graph/NodeGraph.h"
#include "GraphEditorTab.gen.h"

class GraphEditorView;

/** Dockable panel wrapping a GraphEditorView. Until graphs are assets it edits a demo document:
 *  the example graph on first open, then whatever Ctrl/Cmd+S last wrote next to the binary. */
class GraphEditorTab : public MinorTab {
public:
    ARTIFACT_CLASS();

    GraphEditorTab();

    virtual String GetTabTitle() const override;

    NodeGraph* GetGraph() const { return m_Graph.Get(); }
    GraphEditorView* GetView() const { return m_View; }

private:
    SharedObjectPtr<NodeGraph> m_Graph;
    GraphEditorView* m_View = nullptr;
};

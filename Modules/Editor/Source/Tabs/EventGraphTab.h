#pragma once
#include "MinorTab.h"
#include "EventGraphTab.gen.h"

class GraphEditorView;
class EventGraph;
class VectorImage;
class Asset;

class EventGraphTab : public MinorTab {
public:
    ARTIFACT_CLASS();

    EventGraphTab();

    virtual String GetTabTitle() const override { return "Event Graph"; }
    virtual VectorImage* GetTabIcon() const override;
    virtual void OnBind() override;

    GraphEditorView* GetView() const { return m_View; }
    EventGraph* GetGraph() const { return m_Graph.Get(); }

private:
    void SyncGraph();
    void AdoptGraph();
    void SaveAsset();

    GraphEditorView* m_View = nullptr;
    SharedObjectPtr<EventGraph> m_Graph;
    WeakObjectPtr<Asset> m_Asset;
};

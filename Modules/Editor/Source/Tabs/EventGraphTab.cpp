#include "EventGraphTab.h"
#include "MajorTab.h"
#include "SceneEditorTab.h"
#include "BlueprintEditorTab.h"
#include "Graph/GraphEditorView.h"
#include "UI/UIDockNode.h"
#include "UI/EditorIcons.h"
#include "Assets/NodeAsset.h"
#include "GameFramework/Node.h"
#include "Scripting/EventGraph.h"

EventGraphTab::EventGraphTab() {
    m_View = Add<GraphEditorView>();
    m_View->Fill();
    m_View->OnGraphChanged = [this] { AdoptGraph(); };
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
    m_View->OnSaveRequested = [this] { SaveAsset(); };
}

VectorImage* EventGraphTab::GetTabIcon() const {
    return EditorIcons::GraphEditor();
}

void EventGraphTab::OnBind() {
    SyncGraph();
    MinorTab::OnBind();
    if (m_PendingReveal != 0) {
        m_View->FocusNode(m_PendingReveal);
        m_PendingReveal = 0;
    }
}

void EventGraphTab::RevealNode(uint64_t InNodeId) {
    m_PendingReveal = InNodeId;
    if (UIDockNode* dock = GetDockNode()) {
        dock->SetActiveTab(this);
    }
}

EventGraphTab* EventGraphTab::FindIn(UINode& InMajorTab) {
    for (uint32_t i = 0; i < InMajorTab.GetChildCount(); i++) {
        Node* child = InMajorTab.GetChild((int)i);
        if (EventGraphTab* tab = Cast<EventGraphTab>(child)) {
            return tab;
        }
        if (UINode* node = Cast<UINode>(child)) {
            if (EventGraphTab* nested = FindIn(*node)) {
                return nested;
            }
        }
    }
    return nullptr;
}

void EventGraphTab::SyncGraph() {
    MajorTab* owner = GetMajorTab();
    NodeAsset* asset = owner ? Cast<NodeAsset>(owner->GetEditedAsset()) : nullptr;
    Node* root = owner ? owner->GetAssetRootNode() : nullptr;
    m_View->PaletteContext = root;
    if (!asset) {
        return;
    }
    const bool assetChanged = m_Asset.Get() != asset;
    EventGraph* stored = asset->GetEventGraph();
    if (!assetChanged && (!stored || stored == m_Graph.Get())) {
        return;
    }
    m_Asset = asset;
    m_Graph = stored ? stored : Object::Create<EventGraph>();
    if (root) {
        m_Graph->SelfClass = root->GetClass().Name;
    }
    m_Graph->RefreshNodes();
    m_View->SetGraph(SharedObjectPtr<NodeGraph>(m_Graph.Get()));
}

void EventGraphTab::AdoptGraph() {
    NodeAsset* asset = Cast<NodeAsset>(m_Asset.Get());
    if (asset && m_Graph && asset->GetEventGraph() != m_Graph.Get()) {
        asset->SetEventGraph(m_Graph.Get());
    }
}

void EventGraphTab::SaveAsset() {
    if (SceneEditorTab* scene = Cast<SceneEditorTab>(GetMajorTab())) {
        scene->Save();
    } else if (BlueprintEditorTab* blueprint = Cast<BlueprintEditorTab>(GetMajorTab())) {
        blueprint->Save();
    }
}

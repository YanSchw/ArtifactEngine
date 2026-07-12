#pragma once
#include "GameFramework/UINode.h"
#include "Graph/NodeGraph.h"
#include "GraphEditorView.gen.h"

/** The interactive canvas of the graph editor. */
class GraphEditorView : public UINode {
public:
    ARTIFACT_CLASS();

    GraphEditorView();

    void SetGraph(const SharedObjectPtr<NodeGraph>& InGraph);
    NodeGraph* GetGraph() const { return m_Graph.Get(); }

    /** Centers the view on the selection, or on the whole graph when nothing is selected. */
    void FrameContent();

    /** Invoked on Ctrl/Cmd+S while the cursor is over the view. */
    std::function<void()> OnSaveRequested;

    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;
    virtual void OnPressed(const Vec2& InCursorPos) override;
    virtual void OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) override;
    virtual void OnReleased(bool InInside) override;
    virtual bool OnScroll(const Vec2& InDelta) override;

private:
    enum class DragMode : uint8_t { None, MoveNodes, Connect, Marquee };

    struct PinRef {
        uint64_t NodeId = 0;
        String PinName;
        GraphPinDirection Direction = GraphPinDirection::Input;

        bool IsValid() const { return NodeId != 0; }
        void Reset() { NodeId = 0; PinName.clear(); }
        bool operator==(const PinRef& InOther) const {
            return NodeId == InOther.NodeId && PinName == InOther.PinName && Direction == InOther.Direction;
        }
    };

    struct PinSlot {
        GraphPin* Pin = nullptr;
        Vec2 Center;  // graph space, on the node's edge
    };

    /** A node's geometry in graph space, derived from its title/pins each time it is needed. */
    struct NodeLayout {
        UIRectF Bounds;
        UIRectF Header;
        Array<PinSlot> Slots;
    };

    struct MenuEntry {
        Class NodeClass;
        String Label;
        bool IsHeader = false;
    };

    Vec2 GraphToScreen(const Vec2& InGraphPos) const;
    Vec2 ScreenToGraph(const Vec2& InScreenPos) const;
    static Vec2 SnapToGrid(const Vec2& InGraphPos);

    NodeLayout ComputeNodeLayout(GraphNode& InNode) const;
    GraphNode* FindNodeAt(const Vec2& InGraphPos) const;
    bool FindPinAt(const Vec2& InScreenPos, PinRef& OutPin) const;
    bool ResolvePin(const PinRef& InRef, GraphNode*& OutNode, GraphPin*& OutPin) const;
    bool GetPinCenter(GraphNode& InNode, const GraphPin& InPin, Vec2& OutGraphCenter) const;

    bool IsSelected(uint64_t InNodeId) const;
    void SelectOnly(uint64_t InNodeId);
    void ToggleSelection(uint64_t InNodeId);
    void DeleteSelection();
    void BeginMoveDrag();
    void FinishConnectDrag();
    void FinishMarquee();

    void UpdateHoverPin();
    void UpdatePanning();
    void UpdateShortcuts();

    void OpenContextMenu(const Vec2& InScreenPos);
    void CloseContextMenu();
    void BuildMenuEntries();
    UIRectF GetMenuRect() const;
    int32_t MenuRowAt(const Vec2& InScreenPos) const;
    void ActivateMenuRow(int32_t InRow);

    void PaintGrid(UIDrawList& OutDrawList) const;
    void PaintConnections(UIDrawList& OutDrawList) const;
    void PaintNode(UIDrawList& OutDrawList, GraphNode& InNode) const;
    void PaintPendingWire(UIDrawList& OutDrawList) const;
    void PaintMarquee(UIDrawList& OutDrawList) const;
    void PaintContextMenu(UIDrawList& OutDrawList) const;
    void PaintWire(UIDrawList& OutDrawList, const Vec2& InFromScreen, const Vec2& InToScreen, Vec4 InColorA, Vec4 InColorB) const;

    SharedObjectPtr<NodeGraph> m_Graph;
    Vec2 m_ViewOrigin = Vec2(0.0f);  // graph coords under the widget's top-left corner
    float m_Zoom = 1.0f;

    Array<uint64_t> m_Selection;

    DragMode m_DragMode = DragMode::None;
    Vec2 m_PressScreen = Vec2(0.0f);
    Vec2 m_CursorScreen = Vec2(0.0f);
    bool m_PressMoved = false;
    uint64_t m_PressedNodeId = 0;
    bool m_PressedNodeWasSelected = false;
    Map<uint64_t, Vec2> m_MoveStartPositions;
    PinRef m_ConnectSource;
    PinRef m_HoverPin;
    bool m_HoverPinCompatible = true;

    // Right/middle mouse is not routed through UICanvas, so pan/context-menu state is tracked
    // here from polled buttons.
    bool m_WasPanButtonDown = false;
    bool m_PanCandidate = false;
    bool m_PanIsRightButton = false;
    bool m_Panning = false;
    Vec2 m_PanPressScreen = Vec2(0.0f);
    Vec2 m_LastPanCursor = Vec2(0.0f);

    bool m_MenuOpen = false;
    Vec2 m_MenuScreen = Vec2(0.0f);
    Vec2 m_MenuGraphPos = Vec2(0.0f);
    int32_t m_MenuHoverRow = -1;
    Array<MenuEntry> m_MenuEntries;

    bool m_PendingFrameContent = true;
};

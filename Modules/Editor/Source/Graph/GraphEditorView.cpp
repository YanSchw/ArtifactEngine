#include "GraphEditorView.h"
#include "GraphEditorStyle.h"
#include "ThemedWindow.h"
#include "GameFramework/UICanvas.h"
#include "Rendering/UIDrawList.h"
#include "Assets/Font.h"
#include "InputSystem/KeyboardDevice.h"
#include "InputSystem/KeyCodes.h"
#include "InputSystem/MouseCodes.h"

#include <algorithm>
#include <cmath>

static bool IsAnyKeyHeld(KeyCode InLeft, KeyCode InRight) {
    KeyboardDevice* keyboard = KeyboardDevice::Instance();
    return keyboard && (keyboard->IsPressed(InLeft) || keyboard->IsPressed(InRight));
}
static bool IsShiftHeld() { return IsAnyKeyHeld(KeyCode::LeftShift, KeyCode::RightShift); }
static bool IsAltHeld() { return IsAnyKeyHeld(KeyCode::LeftAlt, KeyCode::RightAlt); }

GraphEditorView::GraphEditorView() {
    Interactable = true;
    ClipChildren = true;
}

void GraphEditorView::SetGraph(const SharedObjectPtr<NodeGraph>& InGraph) {
    m_Graph = InGraph;
    m_Selection.Clear();
    m_DragMode = DragMode::None;
    m_ConnectSource.Reset();
    CloseContextMenu();
    m_PendingFrameContent = true;
}

Vec2 GraphEditorView::GraphToScreen(const Vec2& InGraphPos) const {
    return m_Geometry.Min() + (InGraphPos - m_ViewOrigin) * m_Zoom;
}

Vec2 GraphEditorView::ScreenToGraph(const Vec2& InScreenPos) const {
    return m_ViewOrigin + (InScreenPos - m_Geometry.Min()) / m_Zoom;
}

Vec2 GraphEditorView::SnapToGrid(const Vec2& InGraphPos) {
    const float snap = GraphEditorStyle::SnapSize;
    return Vec2(std::round(InGraphPos.x / snap) * snap, std::round(InGraphPos.y / snap) * snap);
}

void GraphEditorView::FrameContent() {
    if (!m_Graph) {
        return;
    }

    Vec2 boundsMin = Vec2(0.0f);
    Vec2 boundsMax = Vec2(0.0f);
    bool anyNode = false;
    for (const SharedObjectPtr<GraphNode>& node : m_Graph->Nodes) {
        if (!node || (!m_Selection.IsEmpty() && !IsSelected(node->NodeId))) {
            continue;
        }
        const UIRectF bounds = ComputeNodeLayout(*node).Bounds;
        boundsMin = anyNode ? glm::min(boundsMin, bounds.Min()) : bounds.Min();
        boundsMax = anyNode ? glm::max(boundsMax, bounds.Max()) : bounds.Max();
        anyNode = true;
    }

    const Vec2 center = anyNode ? (boundsMin + boundsMax) * 0.5f : Vec2(0.0f);
    m_ViewOrigin = center - m_Geometry.Size * 0.5f / m_Zoom;
}

/* ---------------------------------- Layout ---------------------------------- */

GraphEditorView::NodeLayout GraphEditorView::ComputeNodeLayout(GraphNode& InNode) const {
    Font* font = GetDefaultFont();
    const auto measure = [font](const String& InText, float InSize) {
        return font ? font->MeasureText(InText, InSize).x : (float)InText.size() * InSize * 0.55f;
    };

    const Array<GraphPin*> inputs = InNode.GetPins(GraphPinDirection::Input);
    const Array<GraphPin*> outputs = InNode.GetPins(GraphPinDirection::Output);

    float maxInputLabel = 0.0f;
    float maxOutputLabel = 0.0f;
    for (GraphPin* pin : inputs) {
        maxInputLabel = std::max(maxInputLabel, measure(pin->Name, GraphEditorStyle::PinFontSize));
    }
    for (GraphPin* pin : outputs) {
        maxOutputLabel = std::max(maxOutputLabel, measure(pin->Name, GraphEditorStyle::PinFontSize));
    }

    const float titleWidth = measure(InNode.GetTitle(), GraphEditorStyle::TitleFontSize) + 2.0f * GraphEditorStyle::NodePaddingX;
    const float pinsWidth = 2.0f * GraphEditorStyle::PinLabelGap + maxInputLabel + maxOutputLabel + GraphEditorStyle::PinColumnGap;
    const float width = std::max({ GraphEditorStyle::NodeMinWidth, titleWidth, pinsWidth });

    const int32_t rows = std::max(inputs.Size(), outputs.Size());
    const float height = GraphEditorStyle::HeaderHeight + (float)rows * GraphEditorStyle::RowHeight + GraphEditorStyle::NodeBottomPadding;

    NodeLayout layout;
    layout.Bounds = UIRectF(InNode.GetPosition(), Vec2(width, height));
    layout.Header = UIRectF(InNode.GetPosition(), Vec2(width, GraphEditorStyle::HeaderHeight));

    const auto rowCenterY = [&](int32_t InRow) {
        return layout.Bounds.Min().y + GraphEditorStyle::HeaderHeight + ((float)InRow + 0.5f) * GraphEditorStyle::RowHeight;
    };
    for (int32_t i = 0; i < inputs.Size(); i++) {
        layout.Slots.Add({ inputs[i], Vec2(layout.Bounds.Min().x, rowCenterY(i)) });
    }
    for (int32_t i = 0; i < outputs.Size(); i++) {
        layout.Slots.Add({ outputs[i], Vec2(layout.Bounds.Max().x, rowCenterY(i)) });
    }
    return layout;
}

GraphNode* GraphEditorView::FindNodeAt(const Vec2& InGraphPos) const {
    if (!m_Graph) {
        return nullptr;
    }
    for (int32_t i = m_Graph->Nodes.Size() - 1; i >= 0; i--) {
        GraphNode* node = m_Graph->Nodes[i].Get();
        if (node && ComputeNodeLayout(*node).Bounds.Contains(InGraphPos)) {
            return node;
        }
    }
    return nullptr;
}

bool GraphEditorView::FindPinAt(const Vec2& InScreenPos, PinRef& OutPin) const {
    if (!m_Graph) {
        return false;
    }
    const float hitRadius = std::max(GraphEditorStyle::PinHitRadius * m_Zoom, 8.0f);
    for (int32_t i = m_Graph->Nodes.Size() - 1; i >= 0; i--) {
        GraphNode* node = m_Graph->Nodes[i].Get();
        if (!node) {
            continue;
        }
        for (const PinSlot& slot : ComputeNodeLayout(*node).Slots) {
            if (glm::length(GraphToScreen(slot.Center) - InScreenPos) <= hitRadius) {
                OutPin = { node->NodeId, slot.Pin->Name, slot.Pin->Direction };
                return true;
            }
        }
    }
    return false;
}

bool GraphEditorView::ResolvePin(const PinRef& InRef, GraphNode*& OutNode, GraphPin*& OutPin) const {
    OutNode = m_Graph ? m_Graph->FindNode(InRef.NodeId) : nullptr;
    OutPin = OutNode ? OutNode->FindPin(InRef.PinName, InRef.Direction) : nullptr;
    return OutPin != nullptr;
}

bool GraphEditorView::GetPinCenter(GraphNode& InNode, const GraphPin& InPin, Vec2& OutGraphCenter) const {
    const NodeLayout layout = ComputeNodeLayout(InNode);
    for (const PinSlot& slot : layout.Slots) {
        if (slot.Pin == &InPin) {
            OutGraphCenter = slot.Center;
            return true;
        }
    }
    return false;
}

/* --------------------------------- Selection --------------------------------- */

bool GraphEditorView::IsSelected(uint64_t InNodeId) const {
    return m_Selection.Contains(InNodeId);
}

void GraphEditorView::SelectOnly(uint64_t InNodeId) {
    m_Selection.Clear();
    m_Selection.Add(InNodeId);
}

void GraphEditorView::ToggleSelection(uint64_t InNodeId) {
    if (IsSelected(InNodeId)) {
        m_Selection.Remove(InNodeId);
    } else {
        m_Selection.Add(InNodeId);
    }
}

void GraphEditorView::DeleteSelection() {
    if (!m_Graph) {
        return;
    }
    for (uint64_t nodeId : m_Selection) {
        m_Graph->RemoveNode(nodeId);
    }
    m_Selection.Clear();
}

/* ----------------------------------- Input ----------------------------------- */

void GraphEditorView::OnPressed(const Vec2& InCursorPos) {
    m_PressScreen = InCursorPos;
    m_CursorScreen = InCursorPos;
    m_PressMoved = false;
    m_PressedNodeId = 0;

    if (m_MenuOpen) {
        const int32_t row = MenuRowAt(InCursorPos);
        CloseContextMenu();
        if (row >= 0) {
            ActivateMenuRow(row);
            return;
        }
    }
    if (!m_Graph) {
        return;
    }

    // Pins first: they sit on node edges and take priority over the node body.
    PinRef pin;
    if (FindPinAt(InCursorPos, pin)) {
        GraphNode* node = nullptr;
        GraphPin* pinPtr = nullptr;
        if (!ResolvePin(pin, node, pinPtr)) {
            return;
        }
        if (IsAltHeld()) {
            m_Graph->BreakPinConnections(*node, *pinPtr);
            return;
        }
        m_ConnectSource = pin;
        m_DragMode = DragMode::Connect;
        return;
    }

    if (GraphNode* node = FindNodeAt(ScreenToGraph(InCursorPos))) {
        m_PressedNodeId = node->NodeId;
        m_PressedNodeWasSelected = IsSelected(node->NodeId);
        m_Graph->BringToFront(*node);
        if (IsShiftHeld()) {
            ToggleSelection(node->NodeId);
        } else if (!m_PressedNodeWasSelected) {
            SelectOnly(node->NodeId);
        }
        if (IsSelected(node->NodeId)) {
            BeginMoveDrag();
        }
        return;
    }

    if (!IsShiftHeld()) {
        m_Selection.Clear();
    }
    m_DragMode = DragMode::Marquee;
}

void GraphEditorView::BeginMoveDrag() {
    m_DragMode = DragMode::MoveNodes;
    m_MoveStartPositions.Clear();
    for (uint64_t nodeId : m_Selection) {
        if (GraphNode* node = m_Graph->FindNode(nodeId)) {
            m_MoveStartPositions[nodeId] = node->GetPosition();
        }
    }
}

void GraphEditorView::OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) {
    (void)InDelta;
    m_CursorScreen = InCursorPos;
    if (glm::length(InCursorPos - m_PressScreen) > 3.0f) {
        m_PressMoved = true;
    }

    if (m_DragMode == DragMode::MoveNodes && m_Graph) {
        const Vec2 graphDelta = (InCursorPos - m_PressScreen) / m_Zoom;
        for (const auto& [nodeId, startPosition] : m_MoveStartPositions) {
            if (GraphNode* node = m_Graph->FindNode(nodeId)) {
                node->SetPosition(SnapToGrid(startPosition + graphDelta));
            }
        }
    }
}

void GraphEditorView::OnReleased(bool InInside) {
    (void)InInside;
    switch (m_DragMode) {
        case DragMode::MoveNodes:
            // A plain click on an already-selected node collapses the selection to it.
            if (!m_PressMoved && m_PressedNodeWasSelected && !IsShiftHeld()) {
                SelectOnly(m_PressedNodeId);
            }
            break;
        case DragMode::Connect:
            FinishConnectDrag();
            break;
        case DragMode::Marquee:
            FinishMarquee();
            break;
        default:
            break;
    }
    m_DragMode = DragMode::None;
    m_ConnectSource.Reset();
    m_MoveStartPositions.Clear();
}

void GraphEditorView::FinishConnectDrag() {
    PinRef target;
    if (!m_Graph || !FindPinAt(m_CursorScreen, target) || target == m_ConnectSource) {
        return;
    }
    GraphNode* nodeA = nullptr;
    GraphPin* pinA = nullptr;
    GraphNode* nodeB = nullptr;
    GraphPin* pinB = nullptr;
    if (ResolvePin(m_ConnectSource, nodeA, pinA) && ResolvePin(target, nodeB, pinB)) {
        m_Graph->Connect(*nodeA, *pinA, *nodeB, *pinB);
    }
}

void GraphEditorView::FinishMarquee() {
    if (!m_Graph || !m_PressMoved) {
        return;
    }
    const Vec2 a = ScreenToGraph(m_PressScreen);
    const Vec2 b = ScreenToGraph(m_CursorScreen);
    const Vec2 rectMin = glm::min(a, b);
    const Vec2 rectMax = glm::max(a, b);

    for (const SharedObjectPtr<GraphNode>& node : m_Graph->Nodes) {
        if (!node) {
            continue;
        }
        const UIRectF bounds = ComputeNodeLayout(*node).Bounds;
        const bool overlaps = bounds.Min().x <= rectMax.x && rectMin.x <= bounds.Max().x
                           && bounds.Min().y <= rectMax.y && rectMin.y <= bounds.Max().y;
        if (overlaps && !IsSelected(node->NodeId)) {
            m_Selection.Add(node->NodeId);
        }
    }
}

bool GraphEditorView::OnScroll(const Vec2& InDelta) {
    const float newZoom = std::clamp(m_Zoom * std::pow(1.1f, InDelta.y), GraphEditorStyle::ZoomMin, GraphEditorStyle::ZoomMax);
    if (newZoom == m_Zoom) {
        return true;
    }
    // Zoom about the cursor: the graph point under it stays put.
    const Vec2 anchor = ScreenToGraph(m_CursorScreen);
    m_Zoom = newZoom;
    m_ViewOrigin = anchor - (m_CursorScreen - m_Geometry.Min()) / m_Zoom;
    return true;
}

void GraphEditorView::OnUIUpdate(const UIFrameContext& InContext) {
    m_CursorScreen = InContext.CursorPosition;
    UpdateHoverPin();
    // Keyboard and right/middle mouse are polled globally, while an unfocused window's cursor
    // position goes stale inside its bounds — so only the focused window's view may react.
    ThemedWindow* focusedWindow = Cast<ThemedWindow>(Window::GetFocusedWindow());
    if (focusedWindow && focusedWindow->GetCanvas() == GetRootNode()) {
        UpdatePanning();
        UpdateShortcuts();
    }
    m_MenuHoverRow = m_MenuOpen ? MenuRowAt(m_CursorScreen) : -1;
}

void GraphEditorView::UpdateHoverPin() {
    m_HoverPin.Reset();
    m_HoverPinCompatible = true;
    if (m_MenuOpen || !FindPinAt(m_CursorScreen, m_HoverPin)) {
        return;
    }
    if (m_DragMode == DragMode::Connect && m_ConnectSource.IsValid() && !(m_HoverPin == m_ConnectSource)) {
        GraphNode* nodeA = nullptr;
        GraphPin* pinA = nullptr;
        GraphNode* nodeB = nullptr;
        GraphPin* pinB = nullptr;
        if (ResolvePin(m_ConnectSource, nodeA, pinA) && ResolvePin(m_HoverPin, nodeB, pinB)) {
            String reason;
            m_HoverPinCompatible = m_Graph->CanConnect(*nodeA, *pinA, *nodeB, *pinB, reason);
        }
    }
}

void GraphEditorView::UpdatePanning() {
    Window* window = Window::GetFocusedWindow();
    const bool rightDown = window && window->IsMouseButtonDown((int32_t)MouseCode::Right);
    const bool middleDown = window && window->IsMouseButtonDown((int32_t)MouseCode::Middle);
    const bool panDown = rightDown || middleDown;

    if (panDown && !m_WasPanButtonDown && IsHovered() && m_DragMode == DragMode::None) {
        m_PanCandidate = true;
        m_Panning = false;
        m_PanIsRightButton = rightDown;
        m_PanPressScreen = m_CursorScreen;
        m_LastPanCursor = m_CursorScreen;
    }

    if (panDown && m_PanCandidate) {
        if (!m_Panning && glm::length(m_CursorScreen - m_PanPressScreen) > 4.0f) {
            m_Panning = true;
            CloseContextMenu();
        }
        if (m_Panning) {
            m_ViewOrigin -= (m_CursorScreen - m_LastPanCursor) / m_Zoom;
        }
        m_LastPanCursor = m_CursorScreen;
    }

    if (!panDown && m_WasPanButtonDown && m_PanCandidate) {
        if (!m_Panning && m_PanIsRightButton) {
            OpenContextMenu(m_PanPressScreen);
        }
        m_PanCandidate = false;
        m_Panning = false;
    }

    m_WasPanButtonDown = panDown;
}

void GraphEditorView::UpdateShortcuts() {
    KeyboardDevice* keyboard = KeyboardDevice::Instance();
    if (!keyboard || !IsHovered()) {
        return;
    }

    if (keyboard->IsDown(KeyCode::Delete) || keyboard->IsDown(KeyCode::Backspace)) {
        DeleteSelection();
    }
    if (keyboard->IsDown(KeyCode::F)) {
        FrameContent();
    }
    if (keyboard->IsDown(KeyCode::Escape)) {
        CloseContextMenu();
        m_DragMode = DragMode::None;
    }

    const bool commandHeld = IsAnyKeyHeld(KeyCode::LeftControl, KeyCode::RightControl)
                          || IsAnyKeyHeld(KeyCode::LeftSuper, KeyCode::RightSuper);
    if (commandHeld && keyboard->IsDown(KeyCode::S) && OnSaveRequested) {
        OnSaveRequested();
    }
}

/* -------------------------------- Context menu -------------------------------- */

void GraphEditorView::OpenContextMenu(const Vec2& InScreenPos) {
    BuildMenuEntries();
    if (m_MenuEntries.IsEmpty() || !m_Graph) {
        return;
    }
    m_MenuOpen = true;
    m_MenuScreen = InScreenPos;
    m_MenuGraphPos = ScreenToGraph(InScreenPos);
    m_MenuHoverRow = -1;
}

void GraphEditorView::CloseContextMenu() {
    m_MenuOpen = false;
    m_MenuHoverRow = -1;
}

void GraphEditorView::BuildMenuEntries() {
    struct Item {
        String Category;
        String Title;
        Class NodeClass;
    };
    Array<Item> items;
    for (const Class& nodeClass : Class::GetSubclassesOf(GraphNode::StaticClass())) {
        if (nodeClass == GraphNode::StaticClass()) {
            continue;
        }
        // Probe an instance for its menu presentation
        Object* probe = Object::Create(nodeClass);
        if (!probe) {
            continue;
        }
        if (GraphNode* node = probe->As<GraphNode>()) {
            if (node->IsUserCreatable()) {
                items.Add({ node->GetCategory(), node->GetTitle(), nodeClass });
            }
        }
        delete probe;
    }
    items.Sort([](const Item& InA, const Item& InB) {
        return InA.Category == InB.Category ? InA.Title < InB.Title : InA.Category < InB.Category;
    });

    m_MenuEntries.Clear();
    String lastCategory;
    for (const Item& item : items) {
        if (item.Category != lastCategory) {
            m_MenuEntries.Add({ Class::None, item.Category, true });
            lastCategory = item.Category;
        }
        m_MenuEntries.Add({ item.NodeClass, item.Title, false });
    }
}

UIRectF GraphEditorView::GetMenuRect() const {
    float height = 2.0f * GraphEditorStyle::MenuPadding;
    for (const MenuEntry& entry : m_MenuEntries) {
        height += entry.IsHeader ? GraphEditorStyle::MenuHeaderHeight : GraphEditorStyle::MenuRowHeight;
    }
    Vec2 position = m_MenuScreen;
    position = glm::min(position, m_Geometry.Max() - Vec2(GraphEditorStyle::MenuWidth, height));
    position = glm::max(position, m_Geometry.Min());
    return UIRectF(position, Vec2(GraphEditorStyle::MenuWidth, height));
}

int32_t GraphEditorView::MenuRowAt(const Vec2& InScreenPos) const {
    const UIRectF rect = GetMenuRect();
    if (!rect.Contains(InScreenPos)) {
        return -1;
    }
    float y = rect.Min().y + GraphEditorStyle::MenuPadding;
    for (int32_t i = 0; i < m_MenuEntries.Size(); i++) {
        const float rowHeight = m_MenuEntries[i].IsHeader ? GraphEditorStyle::MenuHeaderHeight : GraphEditorStyle::MenuRowHeight;
        if (InScreenPos.y >= y && InScreenPos.y < y + rowHeight) {
            return m_MenuEntries[i].IsHeader ? -1 : i;
        }
        y += rowHeight;
    }
    return -1;
}

void GraphEditorView::ActivateMenuRow(int32_t InRow) {
    if (!m_Graph || InRow < 0 || InRow >= m_MenuEntries.Size() || m_MenuEntries[InRow].IsHeader) {
        return;
    }
    if (GraphNode* node = m_Graph->CreateNode(m_MenuEntries[InRow].NodeClass, SnapToGrid(m_MenuGraphPos))) {
        SelectOnly(node->NodeId);
    }
}

/* ---------------------------------- Painting ---------------------------------- */

void GraphEditorView::Paint(UIDrawList& OutDrawList) {
    if (m_PendingFrameContent && m_Geometry.Size.x > 0.0f) {
        m_PendingFrameContent = false;
        FrameContent();
    }

    OutDrawList.PushClipRect(m_Geometry);
    OutDrawList.AddRect(m_Geometry, GraphEditorStyle::Background, m_WorldMatrix);
    PaintGrid(OutDrawList);
    if (m_Graph) {
        PaintConnections(OutDrawList);
        for (const SharedObjectPtr<GraphNode>& node : m_Graph->Nodes) {
            if (node) {
                PaintNode(OutDrawList, *node);
            }
        }
        PaintPendingWire(OutDrawList);
    }
    PaintMarquee(OutDrawList);
    PaintContextMenu(OutDrawList);
    OutDrawList.PopClipRect();
}

void GraphEditorView::PaintGrid(UIDrawList& OutDrawList) const {
    const float step = GraphEditorStyle::GridSize * m_Zoom;
    const float minorAlpha = std::clamp((step - 4.0f) / 12.0f, 0.0f, 1.0f);
    Vec4 minorColor = GraphEditorStyle::GridMinor;
    minorColor.a *= minorAlpha;

    const auto paintAxis = [&](int InAxis) {
        const float viewMin = m_Geometry.Min()[InAxis];
        const float viewMax = m_Geometry.Max()[InAxis];
        const float firstLine = std::floor(ScreenToGraph(m_Geometry.Min())[InAxis] / GraphEditorStyle::GridSize) * GraphEditorStyle::GridSize;
        for (float line = firstLine; ; line += GraphEditorStyle::GridSize) {
            const float screen = viewMin + (line - ScreenToGraph(m_Geometry.Min())[InAxis]) * m_Zoom;
            if (screen > viewMax) {
                break;
            }
            const bool major = (int64_t)std::llround(line / GraphEditorStyle::GridSize) % GraphEditorStyle::GridMajorEvery == 0;
            if (!major && minorAlpha <= 0.0f) {
                continue;
            }
            const Vec4& color = major ? GraphEditorStyle::GridMajor : minorColor;
            if (InAxis == 0) {
                OutDrawList.AddRect(UIRectF(Vec2(screen, m_Geometry.Min().y), Vec2(1.0f, m_Geometry.Size.y)), color, m_WorldMatrix);
            } else {
                OutDrawList.AddRect(UIRectF(Vec2(m_Geometry.Min().x, screen), Vec2(m_Geometry.Size.x, 1.0f)), color, m_WorldMatrix);
            }
        }
    };
    paintAxis(0);
    paintAxis(1);
}

void GraphEditorView::PaintWire(UIDrawList& OutDrawList, const Vec2& InFromScreen, const Vec2& InToScreen, Vec4 InColorA, Vec4 InColorB) const {
    const float tangent = std::clamp(std::abs(InToScreen.x - InFromScreen.x) * 0.5f, 28.0f * m_Zoom, 240.0f * m_Zoom);
    const Vec2 controlA = InFromScreen + Vec2(tangent, 0.0f);
    const Vec2 controlB = InToScreen - Vec2(tangent, 0.0f);
    const float thickness = std::max(1.0f, GraphEditorStyle::WireThickness * m_Zoom);
    // A darker, slightly wider pass underneath keeps crossing wires readable against each other.
    OutDrawList.AddCubicBezier(InFromScreen, controlA, controlB, InToScreen,
                               thickness + std::max(1.5f, 2.0f * m_Zoom), GraphEditorStyle::WireUnderlay, GraphEditorStyle::WireUnderlay, 0, m_WorldMatrix);
    OutDrawList.AddCubicBezier(InFromScreen, controlA, controlB, InToScreen,
                               thickness, InColorA, InColorB, 0, m_WorldMatrix);
}

void GraphEditorView::PaintConnections(UIDrawList& OutDrawList) const {
    for (const SharedObjectPtr<GraphConnection>& connection : m_Graph->Connections) {
        if (!connection) {
            continue;
        }
        GraphNode* fromNode = m_Graph->FindNode(connection->FromNodeId);
        GraphNode* toNode = m_Graph->FindNode(connection->ToNodeId);
        if (!fromNode || !toNode) {
            continue;
        }
        GraphPin* fromPin = fromNode->FindPin(connection->FromPinName, GraphPinDirection::Output);
        GraphPin* toPin = toNode->FindPin(connection->ToPinName, GraphPinDirection::Input);
        if (!fromPin || !toPin) {
            continue;
        }
        Vec2 fromCenter, toCenter;
        if (!GetPinCenter(*fromNode, *fromPin, fromCenter) || !GetPinCenter(*toNode, *toPin, toCenter)) {
            continue;
        }
        PaintWire(OutDrawList, GraphToScreen(fromCenter), GraphToScreen(toCenter),
                  GraphEditorStyle::PinColor(fromPin->TypeName), GraphEditorStyle::PinColor(toPin->TypeName));
    }
}

void GraphEditorView::PaintPendingWire(UIDrawList& OutDrawList) const {
    if (m_DragMode != DragMode::Connect || !m_ConnectSource.IsValid()) {
        return;
    }
    GraphNode* node = nullptr;
    GraphPin* pin = nullptr;
    Vec2 pinCenter;
    if (!ResolvePin(m_ConnectSource, node, pin) || !GetPinCenter(*node, *pin, pinCenter)) {
        return;
    }

    Vec4 color = GraphEditorStyle::PinColor(pin->TypeName);
    if (m_HoverPin.IsValid() && !(m_HoverPin == m_ConnectSource) && !m_HoverPinCompatible) {
        color = GraphEditorStyle::InvalidTargetWire;
    }
    const Vec2 pinScreen = GraphToScreen(pinCenter);
    if (pin->IsOutput()) {
        PaintWire(OutDrawList, pinScreen, m_CursorScreen, color, color);
    } else {
        PaintWire(OutDrawList, m_CursorScreen, pinScreen, color, color);
    }
}

void GraphEditorView::PaintNode(UIDrawList& OutDrawList, GraphNode& InNode) const {
    Font* font = GetDefaultFont();
    const NodeLayout layout = ComputeNodeLayout(InNode);
    const UIRectF body = UIRectF(GraphToScreen(layout.Bounds.Min()), layout.Bounds.Size * m_Zoom);
    const UIRectF header = UIRectF(body.Position, Vec2(body.Size.x, layout.Header.Size.y * m_Zoom));
    const bool selected = IsSelected(InNode.NodeId);
    const bool paintText = m_Zoom >= GraphEditorStyle::TextCullZoom && font;
    const float corner = GraphEditorStyle::NodeCornerRadius * m_Zoom;

    const auto inflate = [](const UIRectF& InRect, float InAmount) {
        return UIRectF(InRect.Position - Vec2(InAmount), InRect.Size + Vec2(2.0f * InAmount));
    };

    OutDrawList.AddRoundedRect(inflate(UIRectF(body.Position + Vec2(0.0f, 3.0f) * m_Zoom, body.Size), 1.5f * m_Zoom),
                               GraphEditorStyle::NodeShadow, corner + 1.5f * m_Zoom, m_WorldMatrix);

    // Border and selection glow sit underneath the body as inflated rounded rects.
    const float borderWidth = selected ? std::max(1.5f, 1.5f * m_Zoom) : 1.0f;
    if (selected) {
        OutDrawList.AddRoundedRect(inflate(body, 4.0f * m_Zoom), GraphEditorStyle::SelectionGlow, corner + 4.0f * m_Zoom, m_WorldMatrix);
    }
    OutDrawList.AddRoundedRect(inflate(body, borderWidth), selected ? GraphEditorStyle::NodeBorderSelected : GraphEditorStyle::NodeBorder,
                               corner + borderWidth, m_WorldMatrix);

    OutDrawList.AddRoundedRect(body, GraphEditorStyle::NodeBody, corner, m_WorldMatrix);

    const Vec4 accent = InNode.GetAccentColor();
    const Vec4 accentTop = glm::mix(accent, Vec4(1.0f, 1.0f, 1.0f, accent.a), 0.10f);
    const Vec4 accentBottom = glm::mix(accent, Vec4(0.0f, 0.0f, 0.0f, accent.a), 0.30f);
    OutDrawList.AddRoundedRectEx(header, accentTop, accentBottom, corner, corner, 0.0f, 0.0f, m_WorldMatrix);
    OutDrawList.AddRect(UIRectF(Vec2(header.Min().x, header.Max().y), Vec2(header.Size.x, std::max(1.0f, m_Zoom))),
                        GraphEditorStyle::HeaderSeparator, m_WorldMatrix);

    if (paintText) {
        const float titleSize = GraphEditorStyle::TitleFontSize * m_Zoom;
        const Vec2 titleExtent = font->MeasureText(InNode.GetTitle(), titleSize);
        const Vec2 titlePos = Vec2(header.Min().x + GraphEditorStyle::NodePaddingX * 0.75f * m_Zoom,
                                   header.Center().y - titleExtent.y * 0.5f);
        OutDrawList.AddText(font, InNode.GetTitle(), titlePos + Vec2(std::max(1.0f, m_Zoom)), titleSize, GraphEditorStyle::TitleShadow, m_WorldMatrix);
        OutDrawList.AddText(font, InNode.GetTitle(), titlePos, titleSize, GraphEditorStyle::TitleColor, m_WorldMatrix);
    }

    for (const PinSlot& slot : layout.Slots) {
        const Vec2 center = GraphToScreen(slot.Center);
        const bool hovered = m_HoverPin.IsValid() && m_HoverPin.NodeId == InNode.NodeId
                          && m_HoverPin.PinName == slot.Pin->Name && m_HoverPin.Direction == slot.Pin->Direction;
        Vec4 color = GraphEditorStyle::PinColor(slot.Pin->TypeName);
        if (hovered) {
            color = glm::mix(color, Vec4(1.0f), 0.35f);
        }
        const float radius = GraphEditorStyle::PinRadius * m_Zoom * (hovered ? 1.3f : 1.0f);
        if (m_Graph->IsPinConnected(InNode, *slot.Pin)) {
            OutDrawList.AddCircle(center, radius, color, 16, m_WorldMatrix);
        } else {
            OutDrawList.AddRing(center, radius, std::max(1.0f, 1.5f * m_Zoom), color, 16, m_WorldMatrix);
        }

        if (paintText) {
            const float labelSize = GraphEditorStyle::PinFontSize * m_Zoom;
            const Vec2 labelExtent = font->MeasureText(slot.Pin->Name, labelSize);
            const float labelY = center.y - labelExtent.y * 0.5f;
            const float gap = GraphEditorStyle::PinLabelGap * m_Zoom;
            const float labelX = slot.Pin->IsInput() ? center.x + gap : center.x - gap - labelExtent.x;
            OutDrawList.AddText(font, slot.Pin->Name, Vec2(labelX, labelY), labelSize, GraphEditorStyle::PinLabelColor, m_WorldMatrix);
        }
    }
}

void GraphEditorView::PaintMarquee(UIDrawList& OutDrawList) const {
    if (m_DragMode != DragMode::Marquee || !m_PressMoved) {
        return;
    }
    const Vec2 rectMin = glm::min(m_PressScreen, m_CursorScreen);
    const Vec2 rectMax = glm::max(m_PressScreen, m_CursorScreen);
    const UIRectF rect = UIRectF(rectMin, rectMax - rectMin);
    OutDrawList.AddRect(rect, GraphEditorStyle::MarqueeFill, m_WorldMatrix);
    OutDrawList.AddRect(UIRectF(rect.Min(), Vec2(rect.Size.x, 1.0f)), GraphEditorStyle::MarqueeBorder, m_WorldMatrix);
    OutDrawList.AddRect(UIRectF(Vec2(rect.Min().x, rect.Max().y), Vec2(rect.Size.x, 1.0f)), GraphEditorStyle::MarqueeBorder, m_WorldMatrix);
    OutDrawList.AddRect(UIRectF(rect.Min(), Vec2(1.0f, rect.Size.y)), GraphEditorStyle::MarqueeBorder, m_WorldMatrix);
    OutDrawList.AddRect(UIRectF(Vec2(rect.Max().x, rect.Min().y), Vec2(1.0f, rect.Size.y)), GraphEditorStyle::MarqueeBorder, m_WorldMatrix);
}

void GraphEditorView::PaintContextMenu(UIDrawList& OutDrawList) const {
    if (!m_MenuOpen) {
        return;
    }
    Font* font = GetDefaultFont();
    const UIRectF rect = GetMenuRect();
    OutDrawList.AddRoundedRect(UIRectF(rect.Min() + Vec2(0.0f, 3.0f) - Vec2(2.0f), rect.Size + Vec2(4.0f)), GraphEditorStyle::NodeShadow, 9.0f, m_WorldMatrix);
    OutDrawList.AddRoundedRect(UIRectF(rect.Min() - Vec2(1.0f), rect.Size + Vec2(2.0f)), GraphEditorStyle::MenuBorder, 7.0f, m_WorldMatrix);
    OutDrawList.AddRoundedRect(rect, GraphEditorStyle::MenuBackground, 6.0f, m_WorldMatrix);

    float y = rect.Min().y + GraphEditorStyle::MenuPadding;
    for (int32_t i = 0; i < m_MenuEntries.Size(); i++) {
        const MenuEntry& entry = m_MenuEntries[i];
        const float rowHeight = entry.IsHeader ? GraphEditorStyle::MenuHeaderHeight : GraphEditorStyle::MenuRowHeight;
        const UIRectF row = UIRectF(Vec2(rect.Min().x, y), Vec2(rect.Size.x, rowHeight));
        if (!entry.IsHeader && i == m_MenuHoverRow) {
            OutDrawList.AddRoundedRect(row.Deflate(UIPadding(3.0f, 1.0f)), GraphEditorStyle::MenuHover, 4.0f, m_WorldMatrix);
        }
        if (font) {
            const float fontSize = entry.IsHeader ? 11.0f : EditorStyle::FontSize;
            const Vec4& color = entry.IsHeader ? EditorStyle::TextDim : EditorStyle::Text;
            const float indent = entry.IsHeader ? 8.0f : 16.0f;
            const Vec2 extent = font->MeasureText(entry.Label, fontSize);
            OutDrawList.AddText(font, entry.Label, Vec2(row.Min().x + indent, row.Center().y - extent.y * 0.5f), fontSize, color, m_WorldMatrix);
        }
        y += rowHeight;
    }
}

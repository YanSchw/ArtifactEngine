#include "AnimationGraphView.h"
#include "GraphEditorStyle.h"
#include "EditorWindow.h"
#include "UI/EditorDragDrop.h"
#include "UI/EditorIcons.h"
#include "UI/UIContextMenu.h"
#include "UI/UIMenuModel.h"
#include "Animation/AnimationStateMachine.h"
#include "Assets/Animation.h"
#include "Assets/Font.h"
#include "GameFramework/AnimatorNode.h"
#include "Rendering/UIDrawList.h"
#include <algorithm>
#include <cfloat>

static constexpr float s_StateMinWidth = 150.0f;
static constexpr float s_StateHeight = 34.0f;
static constexpr float s_StatePaddingX = 18.0f;
static constexpr float s_StateCorner = 4.0f;
static constexpr float s_StateFontSize = 12.0f;
static constexpr float s_LaneSpacing = 9.0f;
static constexpr float s_ArrowSize = 6.0f;
static constexpr float s_TransitionThickness = 2.0f;
static constexpr float s_LiveBarHeight = 4.0f;
static constexpr float s_DoubleClickSeconds = 0.35f;

static const Vec4 s_DefaultStateColor = HexColor(0xC8641E);
static const Vec4 s_TransitionColor = HexColor(0xBDBDBD);
static const Vec4 s_EntryTransitionColor = HexColor(0xE0822F);
static const Vec4 s_LiveColor = HexColor(0x4AA3FF);

static Vec2 LeaveRect(const Vec2& InInside, const Vec2& InToward, const UIRectF& InRect) {
    const Vec2 direction = InToward - InInside;
    float t = 1.0f;
    for (int32_t axis = 0; axis < 2; axis++) {
        if (direction[axis] > 0.0f) {
            t = std::min(t, (InRect.Max()[axis] - InInside[axis]) / direction[axis]);
        } else if (direction[axis] < 0.0f) {
            t = std::min(t, (InRect.Min()[axis] - InInside[axis]) / direction[axis]);
        }
    }
    return InInside + direction * std::max(t, 0.0f);
}

static bool SamePair(const GraphConnection& InA, const GraphConnection& InB) {
    return (InA.FromNodeId == InB.FromNodeId && InA.ToNodeId == InB.ToNodeId)
        || (InA.FromNodeId == InB.ToNodeId && InA.ToNodeId == InB.FromNodeId);
}

AnimationGraphView::AnimationGraphView() {
    EditorDragDrop::AddDropZone(*this,
        [](Asset* InAsset) { return Cast<Animation>(InAsset) != nullptr; },
        [this](Asset* InAsset, const Vec2& InCursorPos) {
            AnimationStateMachine* machine = GetStateMachine();
            if (!machine) {
                return;
            }
            RecordEdit("Add State");
            const Vec2 position = SnapToGrid(ScreenToGraph(InCursorPos) - Vec2(s_StateMinWidth, s_StateHeight) * 0.5f);
            if (AnimationStateNode* state = machine->AddState(Cast<Animation>(InAsset), position)) {
                SelectOnly(state->NodeId);
                NotifyGraphChanged();
            }
        });
}

AnimationStateMachine* AnimationGraphView::GetStateMachine() const {
    return Cast<AnimationStateMachine>(m_Graph.Get());
}

bool AnimationGraphView::IsLiveNode(uint64_t InNodeId) const {
    AnimatorNode* animator = LiveAnimator.Get();
    return animator && InNodeId != 0 && (animator->GetCurrentStateId() == InNodeId || animator->GetNextStateId() == InNodeId);
}

void AnimationGraphView::OnUIUpdate(const UIFrameContext& InContext) {
    GraphEditorView::OnUIUpdate(InContext);
    m_DoubleClickTimer = std::max(0.0f, m_DoubleClickTimer - InContext.DeltaTime);
}

void AnimationGraphView::OnPressed(const Vec2& InCursorPos) {
    const bool wasConnecting = IsConnecting();
    GraphEditorView::OnPressed(InCursorPos);
    if (wasConnecting) {
        return;
    }

    GraphNode* node = FindNodeAt(ScreenToGraph(InCursorPos));
    const uint64_t nodeId = node ? node->NodeId : 0;
    if (node && nodeId == m_LastPressedNode && m_DoubleClickTimer > 0.0f) {
        m_DoubleClickTimer = 0.0f;
        OpenMotion(*node);
        return;
    }
    m_LastPressedNode = nodeId;
    m_DoubleClickTimer = s_DoubleClickSeconds;
}

void AnimationGraphView::OpenMotion(GraphNode& InNode) {
    AnimationStateNode* state = Cast<AnimationStateNode>(&InNode);
    EditorWindow* window = EditorWindow::FindFor(*this);
    if (state && state->Motion.Get() && window) {
        window->OpenAssetEditor(state->Motion.Get());
    }
}

bool AnimationGraphView::OnSecondaryClick(const Vec2& InCursorPos) {
    if (IsConnecting()) {
        CancelConnect();
        return true;
    }
    if (GraphNode* node = FindNodeAt(ScreenToGraph(InCursorPos))) {
        OpenNodeMenu(*node, InCursorPos);
        return true;
    }
    if (GraphConnection* connection = FindConnectionAt(InCursorPos)) {
        SelectConnection(connection);
        UIMenuModel menu;
        menu.Item("Delete Transition", [view = WeakObjectPtr<AnimationGraphView>(this)] {
            if (view.Get()) {
                view.Get()->DeleteSelection();
            }
        }).Icon(EditorIcons::Delete()).Shortcut("Del");
        UIContextMenu::OpenAt(*this, InCursorPos, menu);
        return true;
    }
    return GraphEditorView::OnSecondaryClick(InCursorPos);
}

void AnimationGraphView::OpenNodeMenu(GraphNode& InNode, const Vec2& InCursorPos) {
    if (!IsSelected(InNode.NodeId)) {
        SelectOnly(InNode.NodeId);
    }
    const WeakObjectPtr<AnimationGraphView> view = this;
    const uint64_t nodeId = InNode.NodeId;
    const auto withNode = [view, nodeId](std::function<void(AnimationGraphView&, GraphNode&)> InAction) {
        return [view, nodeId, InAction] {
            AnimationGraphView* graphView = view.Get();
            GraphNode* node = graphView && graphView->m_Graph ? graphView->m_Graph->FindNode(nodeId) : nullptr;
            if (node) {
                InAction(*graphView, *node);
            }
        };
    };

    AnimationStateNode* state = Cast<AnimationStateNode>(&InNode);
    AnimationStateMachine* machine = GetStateMachine();

    UIMenuModel menu;
    menu.Section(InNode.GetTitle());
    if (InNode.FindPin(AnimationGraphNode::OutPin, GraphPinDirection::Output)) {
        menu.Item("Make Transition", withNode([](AnimationGraphView& InView, GraphNode& InTarget) {
            InView.MakeTransitionFrom(InTarget);
        }));
    }
    if (state && machine) {
        menu.Item("Set as Default State", withNode([](AnimationGraphView& InView, GraphNode& InTarget) {
            InView.RecordEdit("Set Default State");
            InView.GetStateMachine()->SetDefaultState(*Cast<AnimationStateNode>(&InTarget));
            InView.NotifyGraphChanged();
        })).Enabled(machine->GetDefaultState() != state);
        menu.Item("Open Animation", withNode([](AnimationGraphView& InView, GraphNode& InTarget) {
            InView.OpenMotion(InTarget);
        })).Icon(EditorIcons::Animation()).Enabled(state->Motion.Get() != nullptr);
    }
    menu.Separator();
    menu.Item("Delete", withNode([](AnimationGraphView& InView, GraphNode& InTarget) {
        (void)InTarget;
        InView.DeleteSelection();
    })).Icon(EditorIcons::Delete()).Shortcut("Del").Enabled(InNode.IsUserDeletable());

    UIContextMenu::OpenAt(*this, InCursorPos, menu);
}

void AnimationGraphView::MakeTransitionFrom(GraphNode& InNode) {
    const String pin = Cast<AnimationEntryNode>(&InNode) ? AnimationEntryNode::DefaultPin : AnimationGraphNode::OutPin;
    if (InNode.FindPin(pin, GraphPinDirection::Output)) {
        BeginConnect({ InNode.NodeId, pin, GraphPinDirection::Output });
    }
}

GraphEditorView::NodeLayout AnimationGraphView::ComputeNodeLayout(GraphNode& InNode) const {
    Font* font = GetDefaultFont();
    const String title = InNode.GetTitle();
    const float titleWidth = font ? font->MeasureText(title, s_StateFontSize).x : (float)title.size() * s_StateFontSize * 0.55f;

    NodeLayout layout;
    layout.Bounds = UIRectF(InNode.GetPosition(), Vec2(std::max(s_StateMinWidth, titleWidth + 2.0f * s_StatePaddingX), s_StateHeight));
    layout.Header = layout.Bounds;
    return layout;
}

bool AnimationGraphView::FindPinAt(const Vec2& InScreenPos, PinRef& OutPin) const {
    GraphNode* node = IsConnecting() ? FindNodeAt(ScreenToGraph(InScreenPos)) : nullptr;
    if (!node || !node->FindPin(AnimationGraphNode::InPin, GraphPinDirection::Input)) {
        return false;
    }
    OutPin = { node->NodeId, AnimationGraphNode::InPin, GraphPinDirection::Input };
    return true;
}

bool AnimationGraphView::GetTransitionEnds(const GraphConnection& InConnection, Vec2& OutFrom, Vec2& OutTo) const {
    GraphNode* fromNode = m_Graph ? m_Graph->FindNode(InConnection.FromNodeId) : nullptr;
    GraphNode* toNode = m_Graph ? m_Graph->FindNode(InConnection.ToNodeId) : nullptr;
    if (!fromNode || !toNode) {
        return false;
    }

    int32_t lane = 0;
    int32_t lanes = 0;
    for (const SharedObjectPtr<GraphConnection>& other : m_Graph->Connections) {
        if (other && SamePair(*other, InConnection)) {
            lane = other.Get() == &InConnection ? lanes : lane;
            lanes++;
        }
    }

    const UIRectF fromBounds = ComputeNodeLayout(*fromNode).Bounds;
    const UIRectF toBounds = ComputeNodeLayout(*toNode).Bounds;
    const bool forward = fromNode->NodeId < toNode->NodeId;
    const Vec2 axis = forward ? toBounds.Center() - fromBounds.Center() : fromBounds.Center() - toBounds.Center();
    const float length = glm::length(axis);
    const Vec2 normal = length > 0.0f ? Vec2(-axis.y, axis.x) / length : Vec2(0.0f, 1.0f);
    const Vec2 offset = normal * ((float)lane - (float)(lanes - 1) * 0.5f) * s_LaneSpacing;

    const Vec2 start = fromBounds.Center() + offset;
    const Vec2 end = toBounds.Center() + offset;
    OutFrom = GraphToScreen(LeaveRect(start, end, fromBounds));
    OutTo = GraphToScreen(LeaveRect(end, start, toBounds));
    return true;
}

float AnimationGraphView::DistanceToConnection(const GraphConnection& InConnection, const Vec2& InScreenPos) const {
    Vec2 from, to;
    return GetTransitionEnds(InConnection, from, to) ? DistanceToSegment(InScreenPos, from, to) : FLT_MAX;
}

void AnimationGraphView::PaintArrow(UIDrawList& OutDrawList, const Vec2& InFrom, const Vec2& InTo, const Vec4& InColor, float InThickness) const {
    OutDrawList.AddLine(InFrom, InTo, InThickness, InColor, m_WorldMatrix);
    const Vec2 delta = InTo - InFrom;
    const float length = glm::length(delta);
    if (length < 1.0f) {
        return;
    }
    const Vec2 direction = delta / length;
    const Vec2 side = Vec2(-direction.y, direction.x);
    const float size = s_ArrowSize * std::max(m_Zoom, 0.6f);
    const Vec2 middle = (InFrom + InTo) * 0.5f;
    Vec2 points[3] = { middle + direction * size, middle - direction * size + side * size, middle - direction * size - side * size };
    const Vec2 a = points[1] - points[0];
    const Vec2 b = points[2] - points[0];
    if (a.x * b.y - a.y * b.x > 0.0f) {
        std::swap(points[1], points[2]);
    }
    OutDrawList.AddConvexPolyFilled(points, 3, InColor, m_WorldMatrix);
}

void AnimationGraphView::PaintConnection(UIDrawList& OutDrawList, const GraphConnection& InConnection) const {
    Vec2 from, to;
    if (!GetTransitionEnds(InConnection, from, to)) {
        return;
    }
    Vec4 color = s_TransitionColor;
    if (Cast<AnimationEntryNode>(m_Graph->FindNode(InConnection.FromNodeId))) {
        color = s_EntryTransitionColor;
    }
    AnimatorNode* animator = LiveAnimator.Get();
    if (animator && animator->GetCurrentStateId() == InConnection.FromNodeId && animator->GetNextStateId() == InConnection.ToNodeId) {
        color = s_LiveColor;
    }
    if (GetSelectedConnection() == &InConnection) {
        color = GraphEditorStyle::NodeBorderSelected;
    }
    PaintArrow(OutDrawList, from, to, color, std::max(1.0f, s_TransitionThickness * m_Zoom));
}

void AnimationGraphView::PaintPendingWire(UIDrawList& OutDrawList) const {
    if (!IsConnecting() || m_ConnectSources.IsEmpty()) {
        return;
    }
    GraphNode* source = m_Graph ? m_Graph->FindNode(m_ConnectSources[0].NodeId) : nullptr;
    if (!source) {
        return;
    }
    const UIRectF bounds = ComputeNodeLayout(*source).Bounds;
    Vec2 target = ScreenToGraph(m_CursorScreen);
    GraphNode* hovered = m_HoverPin.IsValid() ? m_Graph->FindNode(m_HoverPin.NodeId) : nullptr;
    if (hovered && hovered != source) {
        const UIRectF hoveredBounds = ComputeNodeLayout(*hovered).Bounds;
        target = LeaveRect(hoveredBounds.Center(), bounds.Center(), hoveredBounds);
    }
    const Vec4 color = m_HoverPinCompatible ? GraphEditorStyle::NodeBorderSelected : GraphEditorStyle::InvalidTargetWire;
    PaintArrow(OutDrawList, GraphToScreen(LeaveRect(bounds.Center(), target, bounds)), GraphToScreen(target), color,
               std::max(1.0f, s_TransitionThickness * m_Zoom));
}

void AnimationGraphView::PaintNode(UIDrawList& OutDrawList, GraphNode& InNode) const {
    const UIRectF bounds = ComputeNodeLayout(InNode).Bounds;
    const UIRectF body(GraphToScreen(bounds.Min()), bounds.Size * m_Zoom);
    const float corner = s_StateCorner * m_Zoom;
    const bool selected = IsSelected(InNode.NodeId);
    AnimationStateMachine* machine = GetStateMachine();
    const bool isDefault = machine && machine->GetDefaultState() == &InNode;

    const auto inflate = [](const UIRectF& InRect, float InAmount) {
        return UIRectF(InRect.Position - Vec2(InAmount), InRect.Size + Vec2(2.0f * InAmount));
    };

    OutDrawList.AddRoundedRect(inflate(UIRectF(body.Position + Vec2(0.0f, 3.0f) * m_Zoom, body.Size), 1.0f * m_Zoom),
                               GraphEditorStyle::NodeShadow, corner + m_Zoom, m_WorldMatrix);
    if (selected) {
        OutDrawList.AddRoundedRect(inflate(body, 4.0f * m_Zoom), GraphEditorStyle::SelectionGlow, corner + 4.0f * m_Zoom, m_WorldMatrix);
    }
    const float border = selected ? std::max(1.5f, 2.0f * m_Zoom) : 1.0f;
    OutDrawList.AddRoundedRect(inflate(body, border), selected ? GraphEditorStyle::NodeBorderSelected : GraphEditorStyle::NodeBorder,
                               corner + border, m_WorldMatrix);

    const Vec4 fill = isDefault ? s_DefaultStateColor : InNode.GetAccentColor();
    OutDrawList.AddRoundedRectEx(body, glm::mix(fill, Vec4(1.0f, 1.0f, 1.0f, fill.a), 0.08f),
                                 glm::mix(fill, Vec4(0.0f, 0.0f, 0.0f, fill.a), 0.22f), corner, corner, corner, corner, m_WorldMatrix);

    AnimatorNode* animator = LiveAnimator.Get();
    if (animator && animator->GetCurrentStateId() == InNode.NodeId) {
        const float progress = animator->GetNormalizedTime() - std::floor(animator->GetNormalizedTime());
        const float height = s_LiveBarHeight * m_Zoom;
        OutDrawList.AddRect(UIRectF(Vec2(body.Min().x, body.Max().y - height), Vec2(body.Size.x * progress, height)),
                            s_LiveColor, m_WorldMatrix);
    } else if (IsLiveNode(InNode.NodeId)) {
        OutDrawList.AddRoundedRectOutline(body, s_LiveColor, corner, std::max(1.0f, m_Zoom), m_WorldMatrix);
    }

    Font* font = GetDefaultFont();
    if (!font || m_Zoom < GraphEditorStyle::TextCullZoom) {
        return;
    }
    const String title = InNode.GetTitle();
    const float size = s_StateFontSize * m_Zoom;
    const Vec2 extent = font->MeasureText(title, size);
    const Vec2 position = body.Center() - extent * 0.5f;
    OutDrawList.AddText(font, title, position + Vec2(std::max(1.0f, m_Zoom)), size, GraphEditorStyle::TitleShadow, m_WorldMatrix);
    OutDrawList.AddText(font, title, position, size, GraphEditorStyle::TitleColor, m_WorldMatrix);
}

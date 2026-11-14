#pragma once
#include "GraphEditorView.h"
#include "AnimationGraphView.gen.h"

class AnimationStateMachine;
class AnimatorNode;

class AnimationGraphView : public GraphEditorView {
public:
    ARTIFACT_CLASS();

    AnimationGraphView();

    WeakObjectPtr<AnimatorNode> LiveAnimator;

    AnimationStateMachine* GetStateMachine() const;

    virtual void OnPressed(const Vec2& InCursorPos) override;
    virtual bool OnSecondaryClick(const Vec2& InCursorPos) override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;

protected:
    virtual NodeLayout ComputeNodeLayout(GraphNode& InNode) const override;
    virtual bool FindPinAt(const Vec2& InScreenPos, PinRef& OutPin) const override;
    virtual float DistanceToConnection(const GraphConnection& InConnection, const Vec2& InScreenPos) const override;
    virtual void PaintConnection(UIDrawList& OutDrawList, const GraphConnection& InConnection) const override;
    virtual void PaintNode(UIDrawList& OutDrawList, GraphNode& InNode) const override;
    virtual void PaintPendingWire(UIDrawList& OutDrawList) const override;

private:
    bool GetTransitionEnds(const GraphConnection& InConnection, Vec2& OutFrom, Vec2& OutTo) const;
    void PaintArrow(UIDrawList& OutDrawList, const Vec2& InFrom, const Vec2& InTo, const Vec4& InColor, float InThickness) const;
    void OpenNodeMenu(GraphNode& InNode, const Vec2& InCursorPos);
    void MakeTransitionFrom(GraphNode& InNode);
    void OpenMotion(GraphNode& InNode);
    bool IsLiveNode(uint64_t InNodeId) const;

    uint64_t m_LastPressedNode = 0;
    float m_DoubleClickTimer = 0.0f;
};

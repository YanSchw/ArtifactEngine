#pragma once
#include "GameFramework/Node.h"
#include "UILayout.h"
#include "UIInput.h"
#include "InputSystem/CursorIcon.h"
#include "UINode.gen.h"

class UIDrawList;
class Font;

/** Base class for all UI. A UINode is a Node with a rect transform (Anchor/Pivot/Position/Size)
 *  plus CSS-style margin/padding and optional Split (row/column) child layout. The tree is
 *  re-laid-out and re-painted every frame, so you configure a node by setting its public fields
 *  directly — there are no setters to call. A tree is rooted in a UICanvas, which defines the
 *  rect everything lays out in and how it reaches the screen (overlay or in-world).
 *
 *  Setup is meant to be terse:
 *      UICanvas* canvas = new UICanvas();              // fills the viewport (overlay mode)
 *      UIButton* btn    = canvas->Add<UIButton>();     // create + attach a typed child
 *      btn->Center({ 220, 56 });                       // fixed size, centered in the parent
 *
 *  The rect model: Anchor is the reference point in the parent's content rect (fractions [0..1]),
 *  Pivot is the node's own reference point (0.5,0.5 = center), and the node is placed so its
 *  Pivot lands on Anchor + Position. Position and Size are unit-aware (UIValue: fraction of the
 *  parent + pixels, composable with +/-), which covers fixed, stretched and mixed layouts alike:
 *      node->Size = { 1.0_rel - 40.0_px, 22.0_px };   // parent width minus 40px, 22px tall
 *  Padding shrinks the rect its children live in.
 *
 *  Subclasses override Paint() to draw and the On* event virtuals to handle input. */
class UINode : public Node {
public:
    ARTIFACT_CLASS();

    // Layout
    /* Reference point in the parent's content rect, fractions 0..1 (0,0 = top-left, 1,1 = bottom-right). */
    Vec2 Anchor = Vec2(0.5f);
    /* The node's own reference point, fractions 0..1 of its rect; it is placed so Pivot lands on Anchor + Position. */
    Vec2 Pivot = Vec2(0.5f);
    /* Euler degrees about Pivot: X/Y tilt into the screen (perspective), Z spins flat. */
    Vec3 Rotation = Vec3(0.0f);
    /* Offset of Pivot from Anchor. */
    UIVec2 Position;
    /* Extent of the rect. */
    UIVec2 Size = Vec2(100.0f);
    /* Per-edge inset shrinking the rect children lay out in. */
    UIPadding Padding;
    /* Takes part in input routing: hover/press/click under the cursor, focusable in Focus mode. */
    bool Interactable = false;
    /* Pointer shape shown while this node is the topmost interactable hit (or holds the pointer capture). */
    CursorIcon Cursor = CursorIcon::Arrow;
    /* Clip children (painting and hit-testing) to this node's content rect. */
    bool ClipChildren = false;

    /** Create a child of type T, attach it, and return it typed. */
    template<typename T>
    T* Add() { return CreateChild(T::StaticClass())->template As<T>(); }

    /** Preset: stretch to fill the parent's content rect. Returns this. */
    UINode* Fill() {
        Anchor = Pivot = Vec2(0.0f); Position = Vec2(0.0f); Size = { 1.0_rel, 1.0_rel };
        return this;
    }
    /** Preset: fixed size (per-axis pixels or relative), centered in the parent. Returns this. */
    UINode* Center(const UIVec2& InSize) {
        Anchor = Pivot = Vec2(0.5f); Position = Vec2(0.0f); Size = InSize;
        return this;
    }

    /** Final rect in the node's own (unrotated) space, valid after layout. Combine with
     *  GetWorldMatrix() for on-screen corners when the node (or an ancestor) is rotated. */
    const UIRectF& GetGeometry() const { return m_Geometry; }
    /** Geometry minus padding — where children/content live. */
    UIRectF GetContentRect() const { return m_Geometry.Deflate(Padding); }
    /** Local pixel-space -> canvas pixel-space transform (accumulates this node's and ancestors'
     *  Rotation, in the z=0 plane; tilts push corners into depth). The projection to screen
     *  happens on the GPU (see UICanvas::BuildProjection). */
    const Mat4& GetWorldMatrix() const { return m_WorldMatrix; }
    /** InPoint is in screen pixels; correct under rotation and perspective. */
    bool HitTest(const Vec2& InPoint) const;

    virtual void Paint(UIDrawList& OutDrawList);
    /** Per-frame tick; input arrives via the event virtuals below instead. */
    virtual void OnUIUpdate(const UIFrameContext& InContext) { (void)InContext; }

    bool IsHovered() const { return m_Hovered; }
    bool IsPressed() const { return m_Pressed; }
    bool IsFocused() const { return m_Focused; }

    /** The canvas this node is rooted in, or null while detached. */
    class UICanvas* GetCanvas() const;
    /** Move the owning canvas's keyboard focus to this node. */
    void RequestFocus();
    /** Clear the owning canvas's focus if this node holds it. */
    void ReleaseFocus();

    /** Input events from the canvas router (see UICanvas::InputMode). A pressed node captures the
     *  pointer until release; OnScroll and OnNavBack bubble up until a handler returns true. */
    virtual void OnHoverChanged(bool InHovered) { (void)InHovered; }
    virtual void OnPressed(const Vec2& InCursorPos) { (void)InCursorPos; }
    virtual void OnReleased(bool InInside) { (void)InInside; }
    virtual void OnClick() { }
    virtual void OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) { (void)InCursorPos; (void)InDelta; }
    virtual bool OnScroll(const Vec2& InDelta) { (void)InDelta; return false; }
    virtual void OnFocusChanged(bool InFocused) { (void)InFocused; }
    virtual bool OnNavBack() { return false; }

    /** Runs every frame before layout to push current state into fields */
    std::function<void()> Bind;
    /** Overridden by dynamic containers (UIForEach/UIIf) to reconcile children. */
    virtual void OnBind() { if (Bind) Bind(); }

    static void SetDefaultFont(Font* InFont) { s_DefaultFont = InFont; }
    static Font* GetDefaultFont() { return s_DefaultFont; }

protected:
    // Frame passes, walked by UICanvas::RunFrame (bind -> layout -> input -> paint).
    void BindTree();
    void Layout(const UIRectF& InParentContentRect, const Mat4& InParentWorld = Mat4(1.0f));
    void PaintTree(UIDrawList& OutDrawList);
    void UpdateTree(const UIFrameContext& InContext);

    // Set by UICanvas each frame so hit-testing projects exactly like the GPU.
    static void SetViewProjection(const Mat4& InProjection, float InViewportW, float InViewportH) {
        s_ViewProjection = InProjection; s_ViewportW = InViewportW; s_ViewportH = InViewportH;
    }
    static Vec2 ProjectToScreen(const Vec3& InCanvasPixelPos);
    Vec2 LocalToScreen(const Vec2& InLocalPoint) const;

    /** Lay out children within the content rect. Base gives each child the whole rect. */
    virtual void LayoutChildren(const UIRectF& InContent);
    static void LayoutChild(UINode& InChild, const UIRectF& InRect, const Mat4& InParentWorld) {
        InChild.Layout(InRect, InParentWorld);
    }
    Array<UINode*> CollectUIChildren() const;

    /** Paint after all children, outside any clip. */
    virtual void PaintOverlay(UIDrawList& OutDrawList) { (void)OutDrawList; }
    /** Hit-test what PaintOverlay draws; since overlays paint above all children, a hit here
     *  beats any child in input routing. */
    virtual bool HitTestOverlay(const Vec2& InPoint) const { (void)InPoint; return false; }
    bool HitTestRect(const UIRectF& InRect, const Vec2& InPoint) const;

    UIRectF ComputeGeometry(const UIRectF& InParentContentRect) const;
    UIRectF m_Geometry;
    Mat4 m_WorldMatrix = Mat4(1.0f);
    bool m_Hovered = false;
    bool m_Pressed = false;
    bool m_Focused = false;

    inline static Font* s_DefaultFont = nullptr;
    inline static Mat4 s_ViewProjection = Mat4(1.0f);
    inline static float s_ViewportW = 1.0f;
    inline static float s_ViewportH = 1.0f;

    friend class UICanvas;
};

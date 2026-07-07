#pragma once
#include "GameFramework/Node.h"
#include "UILayout.h"
#include "UINode.gen.h"

class UIDrawList;
class Font;

/** Per-frame input passed down the UI tree during the interaction pass (see OnUIUpdate). */
struct UIFrameContext {
    Vec2 MousePosition = Vec2(0.0f);      // pixels, top-left origin
    bool MouseDown = false;               // left button held
    bool MousePressedThisFrame = false;   // left button pressed this frame
    bool MouseReleasedThisFrame = false;  // left button released this frame
    float DeltaTime = 0.0f;
};

/** Base class for all UI. A UINode is a Node with a rect transform: Unity-style anchors/pivot
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
 *  Anchors are fractions [0..1] of the parent's content rect. When AnchorMin == AnchorMax the
 *  node has a fixed size (SizeDelta) positioned at that anchor; when they differ it stretches to
 *  span the anchors and SizeDelta grows/shrinks that span. Pivot is the node's own reference
 *  point (0.5,0.5 = center). Padding shrinks the rect its children live in; Margin insets the
 *  node within its own slot. Set LayoutMode to SplitX/SplitY to auto-arrange children as a
 *  row/column (with Gap between them) instead of anchoring each individually.
 *
 *  Subclasses override Paint() to draw and OnUIUpdate() to react to input. */
class UINode : public Node {
public:
    ARTIFACT_CLASS();

    // Layout — set directly; recomputed every frame.
    Vec2 AnchorMin = Vec2(0.0f);
    Vec2 AnchorMax = Vec2(0.0f);
    Vec2 Pivot = Vec2(0.5f);
    Vec3 Rotation = Vec3(0.0f);           // degrees, euler: X/Y tilt into the screen (perspective), Z spins. About Pivot.
    Vec2 AnchoredPosition = Vec2(0.0f);
    Vec2 SizeDelta = Vec2(100.0f);
    UIEdges Margin;
    UIEdges Padding;
    UILayoutMode LayoutMode = UILayoutMode::None;
    float Gap = 0.0f;
    Vec4 BackgroundColor = Vec4(0.0f);   // painted only when alpha > 0

    /** Create a child of type T, attach it, and return it typed. */
    template<typename T>
    T* Add() { return CreateChild(T::StaticClass())->template As<T>(); }

    /** Preset: stretch to fill the parent's content rect (optionally inset by margin). Returns this. */
    UINode* Fill(const UIEdges& InMargin = UIEdges()) {
        AnchorMin = Vec2(0.0f); AnchorMax = Vec2(1.0f); SizeDelta = Vec2(0.0f); Margin = InMargin;
        return this;
    }
    /** Preset: fixed size, centered in the parent. Returns this. */
    UINode* Center(const Vec2& InSize) {
        AnchorMin = AnchorMax = Pivot = Vec2(0.5f); AnchoredPosition = Vec2(0.0f); SizeDelta = InSize;
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

    /** Draw this node. Base paints BackgroundColor when opaque; call it from overrides for the bg. */
    virtual void Paint(UIDrawList& OutDrawList);
    /** React to input for this node. */
    virtual void OnUIUpdate(const UIFrameContext& InContext) { (void)InContext; }

    /** Declarative data binding: run every frame (before layout) to push current state into fields,
     *  e.g. `label.Bind = [&]{ label.Text = std::to_string(count); };`. See UIBuilder.h / UIForEach / UIIf. */
    std::function<void()> Bind;
    /** Applies this node's binding. Overridden by dynamic containers (UIForEach/UIIf) to reconcile children. */
    virtual void OnBind() { if (Bind) Bind(); }

    // Font used by UILabel/UIButton when they don't specify their own. Set once at startup.
    static void SetDefaultFont(Font* InFont) { s_DefaultFont = InFont; }
    static Font* GetDefaultFont() { return s_DefaultFont; }

    // Perspective distance in canvas units used to project tilted UI to screen. Larger = flatter.
    static void SetPerspective(float InDistance) { s_Perspective = InDistance; }
    static float GetPerspective() { return s_Perspective; }
    // Set by UIRenderer each frame (from UICanvas::BuildProjection) so hit-testing projects
    // exactly like the GPU does, whatever the canvas render mode.
    static void SetViewProjection(const Mat4& InProjection, float InViewportW, float InViewportH) {
        s_ViewProjection = InProjection; s_ViewportW = InViewportW; s_ViewportH = InViewportH;
    }
    /** Project a canvas pixel-space point (post-transform, may have depth) to screen pixels. */
    static Vec2 ProjectToScreen(const Vec3& InCanvasPixelPos);

    // Called by UIRenderer each frame (BindTree runs first, before Layout).
    void BindTree();
    void Layout(const UIRectF& InParentContentRect, const Mat4& InParentWorld = Mat4(1.0f));
    void PaintTree(UIDrawList& OutDrawList);
    void UpdateTree(const UIFrameContext& InContext);

protected:
    UIRectF ComputeGeometry(const UIRectF& InParentContentRect) const;
    // Lay out with an already-resolved geometry (used by split layout, which fills each slot).
    void LayoutSelf(const UIRectF& InResolvedGeometry, const Mat4& InParentWorld);
    UIRectF m_Geometry;
    Mat4 m_WorldMatrix = Mat4(1.0f);

    inline static Font* s_DefaultFont = nullptr;
    inline static float s_Perspective = 1000.0f;
    inline static Mat4 s_ViewProjection = Mat4(1.0f);
    inline static float s_ViewportW = 1.0f;
    inline static float s_ViewportH = 1.0f;
};

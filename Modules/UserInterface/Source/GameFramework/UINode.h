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
 *  Subclasses override Paint() to draw and OnUIUpdate() to react to input. */
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
    /* RGBA 0..1; painted only when alpha > 0. */
    Vec4 BackgroundColor = Vec4(0.0f);

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

protected:
    // The frame passes, walked by UICanvas::RunFrame (bind -> layout -> input -> paint).
    void BindTree();
    void Layout(const UIRectF& InParentContentRect, const Mat4& InParentWorld = Mat4(1.0f));
    void PaintTree(UIDrawList& OutDrawList);
    void UpdateTree(const UIFrameContext& InContext);

    // Set by UICanvas each frame so hit-testing projects exactly like the GPU does,
    // whatever the canvas render mode.
    static void SetViewProjection(const Mat4& InProjection, float InViewportW, float InViewportH) {
        s_ViewProjection = InProjection; s_ViewportW = InViewportW; s_ViewportH = InViewportH;
    }
    /** Project a canvas pixel-space point (post-transform, may have depth) to screen pixels. */
    static Vec2 ProjectToScreen(const Vec3& InCanvasPixelPos);

    /** Lay out this node's children within the given content rect. The base hands every child
     *  the whole rect (each positions itself via its own Anchor/Position/Size); UIStack
     *  overrides this to arrange them into a row/column. */
    virtual void LayoutChildren(const UIRectF& InContent);
    /** For LayoutChildren overrides: recurse into one child with the rect it should lay out in. */
    static void LayoutChild(UINode& InChild, const UIRectF& InRect, const Mat4& InParentWorld) {
        InChild.Layout(InRect, InParentWorld);
    }
    Array<UINode*> CollectUIChildren() const;

    UIRectF ComputeGeometry(const UIRectF& InParentContentRect) const;
    UIRectF m_Geometry;
    Mat4 m_WorldMatrix = Mat4(1.0f);

    inline static Font* s_DefaultFont = nullptr;
    inline static Mat4 s_ViewProjection = Mat4(1.0f);
    inline static float s_ViewportW = 1.0f;
    inline static float s_ViewportH = 1.0f;
};

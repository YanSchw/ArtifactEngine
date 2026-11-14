#pragma once
#include "UINode.h"
#include "Object/Pointer.h"
#include "Object/Enum.h"
#include "UICanvas.gen.h"

class CameraNode;
class UIDrawList;

/** How a canvas reaches the screen.
 *  Overlay — drawn directly in screen space on top of everything, ignoring any scene camera.
 *  World   — the canvas is a plane placed in the 3D world (WorldPosition/WorldRotation, sized
 *            CanvasSize / PixelsPerUnit units) and projected through Camera. It is still drawn
 *            in the UI pass, so scene geometry does not occlude it. */
ARTIFACT_ENUM();
enum class UICanvasRenderMode : uint8_t { Overlay = 0, World = 1 };

/** How an Overlay canvas maps its pixels to the viewport.
 *  ConstantPixelSize   — one canvas pixel is one screen pixel, times ScaleFactor.
 *  ScaleWithScreenSize — the canvas is scaled relative to ReferenceResolution, blending between
 *                        width- and height-relative scale via MatchWidthHeight (Unity-style). */
ARTIFACT_ENUM();
enum class UICanvasScaleMode : uint8_t { ConstantPixelSize = 0, ScaleWithScreenSize = 1 };

/** The root of a UI tree: every UINode you want rendered lives under a canvas, and the canvas
 *  rect is the area its children lay out in. Hand it to UIRenderer::Render() each frame. */
class UICanvas : public UINode {
public:
    ARTIFACT_CLASS();

    UICanvas();

    PROPERTY()
    UICanvasRenderMode RenderMode = UICanvasRenderMode::Overlay;

    // Scaling (Overlay mode).
    PROPERTY()
    UICanvasScaleMode ScaleMode = UICanvasScaleMode::ConstantPixelSize;
    PROPERTY()
    float ScaleFactor = 1.0f;                          // ConstantPixelSize multiplier
    PROPERTY()
    Vec2 ReferenceResolution = Vec2(1920.0f, 1080.0f); // ScaleWithScreenSize design resolution
    PROPERTY(Range = (0, 1))
    float MatchWidthHeight = 0.5f;                     // 0 = match width, 1 = match height

    // Placement (World mode).
    PROPERTY()
    Vec2 CanvasSize = Vec2(1920.0f, 1080.0f); // canvas rect in canvas pixels
    PROPERTY()
    float PixelsPerUnit = 100.0f;             // canvas pixels per world unit
    PROPERTY()
    Vec3 WorldPosition = Vec3(0.0f);          // canvas centre, world units
    PROPERTY()
    Vec3 WorldRotation = Vec3(0.0f);          // degrees, euler (same order as UINode::Rotation)
    CameraNode* Camera = nullptr;             // camera to project through; identity view when null

    // Perspective distance in canvas units for tilted nodes (Overlay mode). Larger = flatter.
    PROPERTY()
    float Perspective = 1000.0f;

    PROPERTY()
    UIInputMode InputMode = UIInputMode::Cursor;

    FUNCTION(Pure)
    UINode* GetFocusedNode() const { return m_FocusedNode.Get(); }
    FUNCTION(Pure)
    UINode* GetHoveredNode() const { return m_HoveredNode.Get(); }
    FUNCTION()
    void SetFocus(UINode* InNode);

    /** Pointer shape requested by the topmost hit (or captured) node this frame. */
    CursorIcon GetDesiredCursor() const { return m_DesiredCursor; }

    /** Disables InNode now and deletes it at the start of the next frame. */
    FUNCTION()
    void DestroyDeferred(UINode* InNode);

    /** Runs one UI frame — UpdateFrame, then PaintFrame. */
    Mat4 RunFrame(const Vec2& InViewportSize, const UIFrameContext& InContext, UIDrawList& OutDrawList);
    /** Bind, layout, input and update, without painting. */
    void UpdateFrame(const Vec2& InViewportSize, const UIFrameContext& InContext);
    /** Paints the tree as UpdateFrame left it, returning the canvas->clip projection to render it with. */
    Mat4 PaintFrame(UIDrawList& OutDrawList);

private:
    float ComputeScaleFactor(const Vec2& InViewportSize) const;
    UIRectF ComputeCanvasRect(const Vec2& InViewportSize) const;
    /** Canvas pixel space -> clip space; uploaded to the GPU and mirrored by ProjectToScreen. */
    Mat4 BuildProjection(const Vec2& InViewportSize) const;

    void RouteInput(const UIFrameContext& InContext);
    /** Picks m_DesiredCursor from the final layout, after update handlers refreshed Cursor. */
    void ResolveCursor(const UIFrameContext& InContext);
    void RouteCursor(const UIFrameContext& InContext);
    void RouteFocus(const UIFrameContext& InContext);
    void SetHovered(UINode* InNode);
    /** Topmost enabled node under the point in paint order (respecting ClipChildren). */
    static UINode* HitTestTopmost(UINode* InNode, const Vec2& InPoint, bool InInteractableOnly);
    UINode* FindNavTarget(UINode* InFrom, UINavDirection InDirection) const;

    WeakObjectPtr<UINode> m_HoveredNode;
    WeakObjectPtr<UINode> m_CapturedNode;
    WeakObjectPtr<UINode> m_FocusedNode;
    WeakObjectPtr<UINode> m_SecondaryNode;
    Array<WeakObjectPtr<UINode>> m_PendingDestroy;
    Vec2 m_LastCursor = Vec2(0.0f);
    Mat4 m_Projection = Mat4(1.0f);
    Vec2 m_ViewportSize = Vec2(1.0f);
    Vec2 m_SecondaryPress = Vec2(0.0f);
    CursorIcon m_DesiredCursor = CursorIcon::Arrow;
};

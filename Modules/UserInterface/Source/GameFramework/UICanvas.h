#pragma once
#include "UINode.h"
#include "Object/Pointer.h"
#include "UICanvas.gen.h"

class CameraNode;
class UIDrawList;

/** How a canvas reaches the screen.
 *  Overlay — drawn directly in screen space on top of everything, ignoring any scene camera.
 *  World   — the canvas is a plane placed in the 3D world (WorldPosition/WorldRotation, sized
 *            CanvasSize / PixelsPerUnit units) and projected through Camera. It is still drawn
 *            in the UI pass, so scene geometry does not occlude it. */
enum class UICanvasRenderMode : uint8_t { Overlay = 0, World = 1 };

/** How an Overlay canvas maps its pixels to the viewport.
 *  ConstantPixelSize   — one canvas pixel is one screen pixel, times ScaleFactor.
 *  ScaleWithScreenSize — the canvas is scaled relative to ReferenceResolution, blending between
 *                        width- and height-relative scale via MatchWidthHeight (Unity-style). */
enum class UICanvasScaleMode : uint8_t { ConstantPixelSize = 0, ScaleWithScreenSize = 1 };

/** The root of a UI tree: every UINode you want rendered lives under a canvas, and the canvas
 *  rect is the area its children lay out in. Hand it to UIRenderer::Render() each frame. */
class UICanvas : public UINode {
public:
    ARTIFACT_CLASS();

    UICanvas();

    UICanvasRenderMode RenderMode = UICanvasRenderMode::Overlay;

    // Scaling (Overlay mode).
    UICanvasScaleMode ScaleMode = UICanvasScaleMode::ConstantPixelSize;
    float ScaleFactor = 1.0f;                          // ConstantPixelSize multiplier
    Vec2 ReferenceResolution = Vec2(1920.0f, 1080.0f); // ScaleWithScreenSize design resolution
    float MatchWidthHeight = 0.5f;                     // 0 = match width, 1 = match height

    // Placement (World mode).
    Vec2 CanvasSize = Vec2(1920.0f, 1080.0f); // canvas rect in canvas pixels
    float PixelsPerUnit = 100.0f;             // canvas pixels per world unit
    Vec3 WorldPosition = Vec3(0.0f);          // canvas centre, world units
    Vec3 WorldRotation = Vec3(0.0f);          // degrees, euler (same order as UINode::Rotation)
    CameraNode* Camera = nullptr;             // camera to project through; identity view when null

    // Perspective distance in canvas units for tilted nodes (Overlay mode). Larger = flatter.
    float Perspective = 1000.0f;

    UIInputMode InputMode = UIInputMode::Cursor;

    UINode* GetFocusedNode() const { return m_FocusedNode.Get(); }
    void SetFocus(UINode* InNode);

    /** Runs one UI frame — bind, layout, input, paint — filling OutDrawList and returning the
     *  canvas->clip projection it must be rendered with. Called by UIRenderer. */
    Mat4 RunFrame(const Vec2& InViewportSize, const UIFrameContext& InContext, UIDrawList& OutDrawList);

private:
    float ComputeScaleFactor(const Vec2& InViewportSize) const;
    UIRectF ComputeCanvasRect(const Vec2& InViewportSize) const;
    /** Canvas pixel space -> clip space; uploaded to the GPU and mirrored by ProjectToScreen. */
    Mat4 BuildProjection(const Vec2& InViewportSize) const;

    void RouteInput(const UIFrameContext& InContext);
    void RouteCursor(const UIFrameContext& InContext);
    void RouteFocus(const UIFrameContext& InContext);
    void SetHovered(UINode* InNode);
    /** Topmost enabled node under the point in paint order (respecting ClipChildren). */
    static UINode* HitTestTopmost(UINode* InNode, const Vec2& InPoint, bool InInteractableOnly);
    UINode* FindNavTarget(UINode* InFrom, UINavDirection InDirection) const;

    WeakObjectPtr<UINode> m_HoveredNode;
    WeakObjectPtr<UINode> m_CapturedNode;
    WeakObjectPtr<UINode> m_FocusedNode;
    Vec2 m_LastCursor = Vec2(0.0f);
};

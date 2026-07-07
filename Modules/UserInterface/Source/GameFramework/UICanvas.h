#pragma once
#include "UINode.h"
#include "UICanvas.gen.h"

class CameraNode;

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
 *  rect is the area its children lay out in. Hand it to UIRenderer::Render() each frame.
 *
 *  The canvas owns the mapping from canvas pixel space to the screen: in Overlay mode that is
 *  the (optionally scaled) pixel projection; in World mode it is a plane transform through the
 *  scene camera, so the same UI tree becomes a 3D object. Children never notice the difference —
 *  they lay out, paint and hit-test in canvas pixels either way. */
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

    /** Effective canvas-pixel -> screen-pixel scale for this viewport (Overlay mode). */
    float ComputeScaleFactor(const Vec2& InViewportSize) const;
    /** The rect the tree lays out in: viewport / scale (Overlay) or CanvasSize (World). */
    UIRectF ComputeCanvasRect(const Vec2& InViewportSize) const;
    /** Canvas pixel space -> clip space; uploaded to the GPU and mirrored by ProjectToScreen. */
    Mat4 BuildProjection(const Vec2& InViewportSize) const;
};

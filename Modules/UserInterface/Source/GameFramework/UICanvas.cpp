#include "UICanvas.h"
#include "Rendering/UIDrawList.h"
#include "GameFramework/CameraNode.h"
#include <algorithm>
#include <cmath>

UICanvas::UICanvas() {
    Fill();
}

float UICanvas::ComputeScaleFactor(const Vec2& InViewportSize) const {
    if (ScaleMode == UICanvasScaleMode::ScaleWithScreenSize) {
        // Blend the width- and height-relative factors in log space so MatchWidthHeight = 0.5
        // treats "twice as wide" and "twice as tall" symmetrically (Unity's CanvasScaler formula).
        const float logW = std::log2(InViewportSize.x / ReferenceResolution.x);
        const float logH = std::log2(InViewportSize.y / ReferenceResolution.y);
        return std::exp2(glm::mix(logW, logH, MatchWidthHeight));
    }
    return ScaleFactor;
}

UIRectF UICanvas::ComputeCanvasRect(const Vec2& InViewportSize) const {
    if (RenderMode == UICanvasRenderMode::World) {
        return UIRectF(Vec2(0.0f), CanvasSize);
    }
    const float scale = std::max(ComputeScaleFactor(InViewportSize), 1e-4f);
    return UIRectF(Vec2(0.0f), InViewportSize / scale);
}

// Maps canvas pixel-space (x in [0,W], y in [0,H], z = depth from tilt) to clip space with a
// perspective divide about the canvas centre. At z=0 it is the plain pixel->NDC mapping.
static Mat4 BuildOverlayProjection(float InWidth, float InHeight, float InPerspective) {
    Mat4 m(0.0f);
    m[0][0] = 2.0f / InWidth;         // clip.x = 2x/W - 1
    m[1][1] = 2.0f / InHeight;        // clip.y = 2y/H - 1
    m[3][0] = -1.0f;
    m[3][1] = -1.0f;
    m[2][3] = -1.0f / InPerspective;  // clip.w = 1 - z/P
    m[3][3] = 1.0f;
    return m;
}

Mat4 UICanvas::BuildProjection(const Vec2& InViewportSize) const {
    if (RenderMode == UICanvasRenderMode::World) {
        // Canvas pixels (top-left origin, +Y down) -> canvas-local units (centred, +Y up).
        const float pixelsPerUnit = std::max(PixelsPerUnit, 1e-4f);
        const Mat4 pxToLocal =
            glm::scale(Mat4(1.0f), Vec3(1.0f, -1.0f, 1.0f) / pixelsPerUnit) *
            glm::translate(Mat4(1.0f), Vec3(-CanvasSize.x * 0.5f, -CanvasSize.y * 0.5f, 0.0f));

        Mat4 rot(1.0f);
        rot = glm::rotate(rot, glm::radians(WorldRotation.z), Vec3(0.0f, 0.0f, 1.0f));
        rot = glm::rotate(rot, glm::radians(WorldRotation.y), Vec3(0.0f, 1.0f, 0.0f));
        rot = glm::rotate(rot, glm::radians(WorldRotation.x), Vec3(1.0f, 0.0f, 0.0f));
        const Mat4 localToWorld = glm::translate(Mat4(1.0f), WorldPosition) * rot;

        const Mat4 viewProjection = Camera ? Camera->GetViewProjectionMatrix() : Mat4(1.0f);
        return viewProjection * localToWorld * pxToLocal;
    }

    // Overlay: project canvas units straight to the viewport; the scale is implicit in the
    // canvas rect being viewport / scale. Perspective (for tilted nodes) is in canvas units.
    const UIRectF rect = ComputeCanvasRect(InViewportSize);
    return BuildOverlayProjection(rect.Size.x, rect.Size.y, Perspective);
}

Mat4 UICanvas::RunFrame(const Vec2& InViewportSize, const UIFrameContext& InContext, UIDrawList& OutDrawList) {
    const Mat4 projection = BuildProjection(InViewportSize);
    SetViewProjection(projection, InViewportSize.x, InViewportSize.y);

    BindTree();
    Layout(ComputeCanvasRect(InViewportSize));
    UpdateTree(InContext);
    PaintTree(OutDrawList);
    return projection;
}

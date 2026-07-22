#include "UIViewportSurface.h"
#include <algorithm>

UIViewportSurface::UIViewportSurface() {
    Interactable = true;
}

// Parametrizes the cursor along the projected edges of the rect, which stays correct when the
// canvas is scaled or the node sits under a rotation.
Vec2 UIViewportSurface::ToRenderPixel(const Vec2& InScreenPos) const {
    const UIRectF& geometry = GetGeometry();
    const Vec2 topLeft = LocalToScreen(geometry.Min());
    const Vec2 edgeX = LocalToScreen(Vec2(geometry.Max().x, geometry.Min().y)) - topLeft;
    const Vec2 edgeY = LocalToScreen(Vec2(geometry.Min().x, geometry.Max().y)) - topLeft;
    const Vec2 delta = InScreenPos - topLeft;

    const float u = glm::dot(delta, edgeX) / std::max(glm::dot(edgeX, edgeX), 1e-6f);
    const float v = glm::dot(delta, edgeY) / std::max(glm::dot(edgeY, edgeY), 1e-6f);
    return Vec2(u * geometry.Size.x, v * geometry.Size.y);
}

void UIViewportSurface::OnPressed(const Vec2& InCursorPos) {
    if (Pressed) {
        Pressed(ToRenderPixel(InCursorPos));
    }
}

void UIViewportSurface::OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) {
    if (Dragged) {
        Dragged(ToRenderPixel(InCursorPos), InDelta);
    }
}

void UIViewportSurface::OnReleased(bool InInside) {
    if (Released) {
        Released(InInside);
    }
}

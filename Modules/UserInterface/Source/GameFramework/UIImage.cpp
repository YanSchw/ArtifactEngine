#include "UIImage.h"
#include "Rendering/UIDrawList.h"
#include "Rendering/Texture.h"
#include <algorithm>
#include <cmath>

// Point on the rect boundary hit by a ray from the center at the given angle
// (degrees, 0 = up, positive = clockwise on screen).
static Vec2 BoundaryPoint(const Vec2& InCenter, const Vec2& InHalfSize, float InAngleDegrees) {
    const float a = glm::radians(InAngleDegrees);
    const Vec2 dir = Vec2(std::sin(a), -std::cos(a));
    const float sx = InHalfSize.x / std::max(std::abs(dir.x), 1e-5f);
    const float sy = InHalfSize.y / std::max(std::abs(dir.y), 1e-5f);
    return InCenter + dir * std::min(sx, sy);
}

void UIImage::Paint(UIDrawList& OutDrawList) {
    const float amount = std::clamp(FillAmount, 0.0f, 1.0f);
    if (Tint.a <= 0.0f || amount <= 0.0f) {
        return;
    }
    Texture* texture = Image.Get();
    const UIRectF& rect = m_Geometry;

    switch (FillMethod) {
    case UIImageFill::None: {
        OutDrawList.AddImageRect(rect, Tint, texture, m_WorldMatrix);
        break;
    }
    case UIImageFill::Horizontal: {
        const UIRectF part(rect.Position, Vec2(rect.Size.x * amount, rect.Size.y));
        OutDrawList.AddImageRect(part, Tint, texture, m_WorldMatrix, Vec2(0.0f), Vec2(amount, 1.0f));
        break;
    }
    case UIImageFill::Vertical: {
        const float cut = rect.Size.y * (1.0f - amount);
        const UIRectF part(rect.Position + Vec2(0.0f, cut), Vec2(rect.Size.x, rect.Size.y - cut));
        OutDrawList.AddImageRect(part, Tint, texture, m_WorldMatrix, Vec2(0.0f, 1.0f - amount), Vec2(1.0f));
        break;
    }
    case UIImageFill::Radial: {
        const Vec2 center = rect.Center();
        const Vec2 halfSize = rect.Size * 0.5f;
        const float sweep = 360.0f * amount;
        const int steps = std::max(1, (int)std::ceil(sweep / 11.25f));  // 32 segments
        const float sign = FillClockwise ? 1.0f : -1.0f;

        auto uvOf = [&](const Vec2& InPoint) { return (InPoint - rect.Min()) / rect.Size; };
        Vec2 prev = BoundaryPoint(center, halfSize, FillOriginDegrees);
        for (int i = 1; i <= steps; i++) {
            const Vec2 current = BoundaryPoint(center, halfSize, FillOriginDegrees + sign * sweep * (float)i / (float)steps);
            // Winding must match the quads' (see UIDrawList corner order) or culling drops us.
            const Vec2 points[3] = { center, FillClockwise ? current : prev, FillClockwise ? prev : current };
            const Vec2 uvs[3] = { uvOf(points[0]), uvOf(points[1]), uvOf(points[2]) };
            OutDrawList.AddImageTriangle(points, uvs, Tint, texture, m_WorldMatrix);
            prev = current;
        }
        break;
    }
    }
}

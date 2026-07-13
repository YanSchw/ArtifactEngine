#include "UIDrawList.h"
#include "Assets/Font.h"
#include <algorithm>
#include <cmath>
#include <numbers>

UIVertex UIDrawList::MakeVertex(const Vec2& InPos, const Vec2& InUv, const Vec4& InColor, const Mat4& InTransform) const {
    const Vec4 p = InTransform * Vec4(InPos.x, InPos.y, 0.0f, 1.0f);
    UIVertex v;
    v.Position = Vec3(p.x, p.y, p.z);
    v.Color = InColor;
    v.TexCoord = InUv;
    return v;
}

void UIDrawList::ExtendBatch(BatchKind InKind, Texture* InTexture, uint32_t InFirstIndex, uint32_t InIndexCount) {
    if (!m_Batches.IsEmpty() && m_Batches.LastItem().Kind == InKind && m_Batches.LastItem().Tex == InTexture) {
        m_Batches.LastItem().IndexCount += InIndexCount;
    } else {
        m_Batches.Add({ InKind, InFirstIndex, InIndexCount, InTexture });
    }
}

void UIDrawList::AppendQuad(BatchKind InKind, const UIRectF& InRectPx, const Vec2& InUvMin, const Vec2& InUvMax, const Vec4& InColor, const Mat4& InTransform, Texture* InTexture) {
    Vec2 mn = InRectPx.Min();
    Vec2 mx = InRectPx.Max();
    if (mx.x <= mn.x || mx.y <= mn.y) {
        return;
    }

    // Clip against the top rect, remapping UVs to the surviving sub-rect.
    Vec2 uvMin = InUvMin;
    Vec2 uvMax = InUvMax;
    if (!m_ClipStack.IsEmpty()) {
        const UIRectF& clip = m_ClipStack.LastItem();
        const Vec2 cmn = Vec2(std::max(mn.x, clip.Min().x), std::max(mn.y, clip.Min().y));
        const Vec2 cmx = Vec2(std::min(mx.x, clip.Max().x), std::min(mx.y, clip.Max().y));
        if (cmx.x <= cmn.x || cmx.y <= cmn.y) {
            return;
        }
        const Vec2 size = mx - mn;
        const Vec2 t0 = (cmn - mn) / size;
        const Vec2 t1 = (cmx - mn) / size;
        uvMin = InUvMin + (InUvMax - InUvMin) * t0;
        uvMax = InUvMin + (InUvMax - InUvMin) * t1;
        mn = cmn;
        mx = cmx;
    }

    const uint32_t base = (uint32_t)m_Vertices.Size();

    // Winding must match the fullscreen quad (TL, BL, BR, TR) or backface culling drops it.
    m_Vertices.Add(MakeVertex(Vec2(mn.x, mn.y), Vec2(uvMin.x, uvMin.y), InColor, InTransform));
    m_Vertices.Add(MakeVertex(Vec2(mn.x, mx.y), Vec2(uvMin.x, uvMax.y), InColor, InTransform));
    m_Vertices.Add(MakeVertex(Vec2(mx.x, mx.y), Vec2(uvMax.x, uvMax.y), InColor, InTransform));
    m_Vertices.Add(MakeVertex(Vec2(mx.x, mn.y), Vec2(uvMax.x, uvMin.y), InColor, InTransform));

    const uint32_t firstIndex = (uint32_t)m_Indices.Size();
    m_Indices.Add(base + 0); m_Indices.Add(base + 1); m_Indices.Add(base + 2);
    m_Indices.Add(base + 0); m_Indices.Add(base + 2); m_Indices.Add(base + 3);

    ExtendBatch(InKind, InTexture, firstIndex, 6);
}

void UIDrawList::AddRect(const UIRectF& InRectPx, const Vec4& InColor, const Mat4& InTransform) {
    AppendQuad(BatchKind::Solid, InRectPx, Vec2(0.0f), Vec2(0.0f), InColor, InTransform, nullptr);
}

void UIDrawList::AddGlyphQuad(const UIRectF& InRectPx, const Vec2& InUvMin, const Vec2& InUvMax, const Vec4& InColor, const Mat4& InTransform) {
    AppendQuad(BatchKind::Text, InRectPx, InUvMin, InUvMax, InColor, InTransform, nullptr);
}

void UIDrawList::AddImageRect(const UIRectF& InRectPx, const Vec4& InColor, Texture* InTexture, const Mat4& InTransform, const Vec2& InUvMin, const Vec2& InUvMax) {
    if (!InTexture) {
        AddRect(InRectPx, InColor, InTransform);
        return;
    }
    AppendQuad(BatchKind::Image, InRectPx, InUvMin, InUvMax, InColor, InTransform, InTexture);
}

void UIDrawList::AddImageTriangle(const Vec2 InPoints[3], const Vec2 InUvs[3], const Vec4& InColor, Texture* InTexture, const Mat4& InTransform) {
    // Coarse clip: drop only when the whole AABB misses the clip rect.
    if (!m_ClipStack.IsEmpty()) {
        const UIRectF& clip = m_ClipStack.LastItem();
        const float minX = std::min({ InPoints[0].x, InPoints[1].x, InPoints[2].x });
        const float maxX = std::max({ InPoints[0].x, InPoints[1].x, InPoints[2].x });
        const float minY = std::min({ InPoints[0].y, InPoints[1].y, InPoints[2].y });
        const float maxY = std::max({ InPoints[0].y, InPoints[1].y, InPoints[2].y });
        if (maxX <= clip.Min().x || minX >= clip.Max().x || maxY <= clip.Min().y || minY >= clip.Max().y) {
            return;
        }
    }

    const uint32_t base = (uint32_t)m_Vertices.Size();
    for (int i = 0; i < 3; i++) {
        m_Vertices.Add(MakeVertex(InPoints[i], InUvs[i], InColor, InTransform));
    }
    const uint32_t firstIndex = (uint32_t)m_Indices.Size();
    m_Indices.Add(base + 0); m_Indices.Add(base + 1); m_Indices.Add(base + 2);

    ExtendBatch(InTexture ? BatchKind::Image : BatchKind::Solid, InTexture, firstIndex, 3);
}

void UIDrawList::AddTriangles(const Vec2* InPositions, const Vec4* InColors, int32_t InVertexCount,
                              const uint32_t* InIndices, int32_t InIndexCount, const Vec4& InTint,
                              const Vec2& InTopLeftPx, const Vec2& InScale, const Mat4& InTransform) {
    if (InVertexCount <= 0 || InIndexCount <= 0) {
        return;
    }

    if (!m_ClipStack.IsEmpty()) {
        Vec2 mn = InTopLeftPx + InPositions[0] * InScale;
        Vec2 mx = mn;
        for (int32_t i = 1; i < InVertexCount; i++) {
            const Vec2 p = InTopLeftPx + InPositions[i] * InScale;
            mn = glm::min(mn, p);
            mx = glm::max(mx, p);
        }
        const UIRectF& clip = m_ClipStack.LastItem();
        if (mx.x <= clip.Min().x || mn.x >= clip.Max().x || mx.y <= clip.Min().y || mn.y >= clip.Max().y) {
            return;
        }
    }

    const uint32_t base = (uint32_t)m_Vertices.Size();
    for (int32_t i = 0; i < InVertexCount; i++) {
        m_Vertices.Add(MakeVertex(InTopLeftPx + InPositions[i] * InScale, Vec2(0.0f), InColors[i] * InTint, InTransform));
    }
    const uint32_t firstIndex = (uint32_t)m_Indices.Size();
    for (int32_t i = 0; i < InIndexCount; i++) {
        m_Indices.Add(base + InIndices[i]);
    }
    ExtendBatch(BatchKind::Solid, nullptr, firstIndex, (uint32_t)InIndexCount);
}

void UIDrawList::AppendConvexPoly(const Vec2* InPoints, int32_t InCount, const Vec4& InColorTop, const Vec4& InColorBottom, const Mat4& InTransform) {
    constexpr int32_t MaxPoints = 64;
    if (InCount < 3 || InCount > MaxPoints - 4) {
        return;
    }

    // Gradient extent is taken from the unclipped polygon, so clipping never shifts the colors.
    float gradientMinY = InPoints[0].y;
    float gradientMaxY = InPoints[0].y;
    for (int32_t i = 1; i < InCount; i++) {
        gradientMinY = std::min(gradientMinY, InPoints[i].y);
        gradientMaxY = std::max(gradientMaxY, InPoints[i].y);
    }
    const float gradientSpan = std::max(gradientMaxY - gradientMinY, 0.0001f);

    // Exact clip via Sutherland-Hodgman: each clip-rect edge can add at most one vertex,
    // and a convex polygon stays convex, so a fan triangulation remains valid.
    Vec2 buffers[2][MaxPoints];
    const Vec2* points = InPoints;
    int32_t count = InCount;

    if (!m_ClipStack.IsEmpty()) {
        const UIRectF& clip = m_ClipStack.LastItem();
        const float bounds[4] = { clip.Min().x, clip.Max().x, clip.Min().y, clip.Max().y };
        for (int32_t edge = 0; edge < 4; edge++) {
            const int32_t axis = edge / 2;         // 0 = x, 1 = y
            const bool keepGreater = (edge % 2) == 0;
            Vec2* out = buffers[edge % 2];
            int32_t outCount = 0;
            for (int32_t i = 0; i < count; i++) {
                const Vec2& a = points[i];
                const Vec2& b = points[(i + 1) % count];
                const float da = keepGreater ? (a[axis] - bounds[edge]) : (bounds[edge] - a[axis]);
                const float db = keepGreater ? (b[axis] - bounds[edge]) : (bounds[edge] - b[axis]);
                if (da >= 0.0f) {
                    out[outCount++] = a;
                }
                if ((da >= 0.0f) != (db >= 0.0f)) {
                    out[outCount++] = a + (b - a) * (da / (da - db));
                }
            }
            points = out;
            count = outCount;
            if (count < 3) {
                return;
            }
        }
    }

    const uint32_t base = (uint32_t)m_Vertices.Size();
    for (int32_t i = 0; i < count; i++) {
        const Vec4 color = glm::mix(InColorTop, InColorBottom, (points[i].y - gradientMinY) / gradientSpan);
        m_Vertices.Add(MakeVertex(points[i], Vec2(0.0f), color, InTransform));
    }
    const uint32_t firstIndex = (uint32_t)m_Indices.Size();
    for (int32_t i = 1; i < count - 1; i++) {
        m_Indices.Add(base); m_Indices.Add(base + i); m_Indices.Add(base + i + 1);
    }
    ExtendBatch(BatchKind::Solid, nullptr, firstIndex, (uint32_t)(count - 2) * 3);
}

void UIDrawList::AddConvexPolyFilled(const Vec2* InPoints, int32_t InCount, const Vec4& InColor, const Mat4& InTransform) {
    AppendConvexPoly(InPoints, InCount, InColor, InColor, InTransform);
}

void UIDrawList::AddRoundedRect(const UIRectF& InRectPx, const Vec4& InColor, float InRadius, const Mat4& InTransform) {
    AddRoundedRectEx(InRectPx, InColor, InColor, InRadius, InRadius, InRadius, InRadius, InTransform);
}

void UIDrawList::AddRoundedRectEx(const UIRectF& InRectPx, const Vec4& InColorTop, const Vec4& InColorBottom,
                                  float InRadiusTL, float InRadiusTR, float InRadiusBR, float InRadiusBL, const Mat4& InTransform) {
    const Vec2 mn = InRectPx.Min();
    const Vec2 mx = InRectPx.Max();
    if (mx.x <= mn.x || mx.y <= mn.y) {
        return;
    }

    const float maxRadius = std::min(InRectPx.Size.x, InRectPx.Size.y) * 0.5f;
    const float halfPi = std::numbers::pi_v<float> * 0.5f;
    // Corners in AddRect's winding (TL, BL, BR, TR); every arc sweeps -90 degrees so the
    // perimeter keeps a consistent rotational direction.
    const float radii[4] = {
        std::clamp(InRadiusTL, 0.0f, maxRadius), std::clamp(InRadiusBL, 0.0f, maxRadius),
        std::clamp(InRadiusBR, 0.0f, maxRadius), std::clamp(InRadiusTR, 0.0f, maxRadius)
    };
    const Vec2 sharpCorners[4] = { mn, Vec2(mn.x, mx.y), mx, Vec2(mx.x, mn.y) };
    const Vec2 arcCenters[4] = {
        mn + Vec2(radii[0], radii[0]),
        Vec2(mn.x + radii[1], mx.y - radii[1]),
        mx - Vec2(radii[2], radii[2]),
        Vec2(mx.x - radii[3], mn.y + radii[3])
    };
    const float startAngles[4] = { -halfPi, std::numbers::pi_v<float>, halfPi, 0.0f };

    Vec2 points[40];
    int32_t count = 0;
    for (int32_t corner = 0; corner < 4; corner++) {
        if (radii[corner] <= 0.5f) {
            points[count++] = sharpCorners[corner];
            continue;
        }
        const int32_t segments = std::clamp((int32_t)(radii[corner] * 0.5f), 2, 8);
        for (int32_t i = 0; i <= segments; i++) {
            const float angle = startAngles[corner] - halfPi * (float)i / (float)segments;
            points[count++] = arcCenters[corner] + Vec2(std::cos(angle), std::sin(angle)) * radii[corner];
        }
    }
    AppendConvexPoly(points, count, InColorTop, InColorBottom, InTransform);
}

void UIDrawList::AddLine(const Vec2& InA, const Vec2& InB, float InThickness, const Vec4& InColor, const Mat4& InTransform) {
    const Vec2 delta = InB - InA;
    const float length = glm::length(delta);
    if (length < 0.001f || InThickness <= 0.0f) {
        return;
    }
    const Vec2 dir = delta / length;
    const Vec2 n = Vec2(-dir.y, dir.x) * (InThickness * 0.5f);
    const Vec2 quad[4] = { InA - n, InA + n, InB + n, InB - n };
    AddConvexPolyFilled(quad, 4, InColor, InTransform);
}

void UIDrawList::AddCubicBezier(const Vec2& InStart, const Vec2& InControlA, const Vec2& InControlB, const Vec2& InEnd,
                                float InThickness, const Vec4& InColorA, const Vec4& InColorB, int32_t InSegments, const Mat4& InTransform) {
    if (InSegments <= 0) {
        const float netLength = glm::length(InControlA - InStart) + glm::length(InControlB - InControlA) + glm::length(InEnd - InControlB);
        InSegments = (int32_t)std::clamp(netLength / 12.0f, 8.0f, 48.0f);
    }

    const auto evaluate = [&](float t) {
        const float u = 1.0f - t;
        return InStart * (u * u * u) + InControlA * (3.0f * u * u * t) + InControlB * (3.0f * u * t * t) + InEnd * (t * t * t);
    };

    // Segments are extended by half a thickness on each end so the joints of adjacent
    // segments overlap instead of leaving wedge-shaped cracks.
    const float capExtent = InThickness * 0.45f;
    Vec2 previous = InStart;
    for (int32_t i = 1; i <= InSegments; i++) {
        const float t = (float)i / (float)InSegments;
        const Vec2 current = evaluate(t);
        const Vec2 delta = current - previous;
        const float length = glm::length(delta);
        if (length > 0.001f) {
            const Vec2 dir = delta / length;
            const Vec4 color = glm::mix(InColorA, InColorB, (t + (float)(i - 1) / (float)InSegments) * 0.5f);
            AddLine(previous - dir * capExtent, current + dir * capExtent, InThickness, color, InTransform);
        }
        previous = current;
    }
}

void UIDrawList::AddCircle(const Vec2& InCenter, float InRadius, const Vec4& InColor, int32_t InSegments, const Mat4& InTransform) {
    constexpr int32_t MaxSegments = 48;
    InSegments = std::clamp(InSegments, 3, MaxSegments);
    Vec2 points[MaxSegments];
    for (int32_t i = 0; i < InSegments; i++) {
        // Clockwise (negative angle step) to match AddRect's winding.
        const float angle = -(float)i / (float)InSegments * 2.0f * std::numbers::pi_v<float>;
        points[i] = InCenter + Vec2(std::cos(angle), std::sin(angle)) * InRadius;
    }
    AddConvexPolyFilled(points, InSegments, InColor, InTransform);
}

void UIDrawList::AddRing(const Vec2& InCenter, float InRadius, float InThickness, const Vec4& InColor, int32_t InSegments, const Mat4& InTransform) {
    InSegments = std::clamp(InSegments, 3, 48);
    const float innerRadius = std::max(0.0f, InRadius - InThickness);
    Vec2 previousOuter, previousInner;
    for (int32_t i = 0; i <= InSegments; i++) {
        const float angle = -(float)i / (float)InSegments * 2.0f * std::numbers::pi_v<float>;
        const Vec2 unit = Vec2(std::cos(angle), std::sin(angle));
        const Vec2 outer = InCenter + unit * InRadius;
        const Vec2 inner = InCenter + unit * innerRadius;
        if (i > 0) {
            const Vec2 quad[4] = { previousOuter, outer, inner, previousInner };
            AddConvexPolyFilled(quad, 4, InColor, InTransform);
        }
        previousOuter = outer;
        previousInner = inner;
    }
}

void UIDrawList::PushClipRect(const UIRectF& InRectPx) {
    UIRectF rect = InRectPx;
    if (!m_ClipStack.IsEmpty()) {
        const UIRectF& top = m_ClipStack.LastItem();
        const Vec2 mn = Vec2(std::max(rect.Min().x, top.Min().x), std::max(rect.Min().y, top.Min().y));
        const Vec2 mx = Vec2(std::min(rect.Max().x, top.Max().x), std::min(rect.Max().y, top.Max().y));
        rect = UIRectF(mn, Vec2(std::max(0.0f, mx.x - mn.x), std::max(0.0f, mx.y - mn.y)));
    }
    m_ClipStack.Add(rect);
}

void UIDrawList::PopClipRect() {
    if (!m_ClipStack.IsEmpty()) {
        m_ClipStack.RemoveLastItem();
    }
}

void UIDrawList::AddText(Font* InFont, const String& InText, const Vec2& InTopLeftPx, float InPixelHeight, const Vec4& InColor, const Mat4& InTransform) {
    if (!InFont || !InFont->IsLoaded()) {
        return;
    }

    const float scale = InFont->GetScaleForPixelHeight(InPixelHeight);
    float penX = InTopLeftPx.x;
    const float baselineY = InTopLeftPx.y + InFont->GetAscentBasePx() * scale;

    for (char c : InText) {
        GlyphInfo glyph;
        if (!InFont->GetGlyph((uint32_t)(unsigned char)c, glyph)) {
            continue;
        }
        if (glyph.SizePx.x > 0.0f && glyph.SizePx.y > 0.0f) {
            const Vec2 topLeft = Vec2(penX + glyph.OffsetPx.x * scale, baselineY + glyph.OffsetPx.y * scale);
            AddGlyphQuad(UIRectF(topLeft, glyph.SizePx * scale), glyph.UvMin, glyph.UvMax, InColor, InTransform);
        }
        penX += glyph.AdvancePx * scale;
    }
}

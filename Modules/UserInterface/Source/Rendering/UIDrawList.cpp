#include "UIDrawList.h"
#include "Assets/Font.h"
#include <algorithm>

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

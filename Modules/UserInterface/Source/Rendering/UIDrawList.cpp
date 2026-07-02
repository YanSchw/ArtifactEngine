#include "UIDrawList.h"
#include "Assets/Font.h"

void UIDrawList::AppendQuad(Array<Vertex>& OutVertices, Array<uint32_t>& OutIndices,
                            const UIRectF& InRectPx, const Vec2& InUvMin, const Vec2& InUvMax, const Vec4& InColor, const Mat4& InTransform) {
    const Vec2 tl = InRectPx.Min();
    const Vec2 br = InRectPx.Max();
    const Vec3 color = Vec3(InColor.r, InColor.g, InColor.b);
    const uint32_t base = (uint32_t)OutVertices.Size();

    // Transform each corner into world pixel space (z=0 plane; tilts add depth). The GPU applies the
    // perspective projection. Corner order matches the engine's fullscreen quad winding: TL, BL, BR, TR.
    auto corner = [&](float x, float y) {
        const Vec4 p = InTransform * Vec4(x, y, 0.0f, 1.0f);
        return Vec3(p.x, p.y, p.z);
    };
    Vertex v0; v0.Position = corner(tl.x, tl.y); v0.Color = color; v0.TexCoord = Vec2(InUvMin.x, InUvMin.y);
    Vertex v1; v1.Position = corner(tl.x, br.y); v1.Color = color; v1.TexCoord = Vec2(InUvMin.x, InUvMax.y);
    Vertex v2; v2.Position = corner(br.x, br.y); v2.Color = color; v2.TexCoord = Vec2(InUvMax.x, InUvMax.y);
    Vertex v3; v3.Position = corner(br.x, tl.y); v3.Color = color; v3.TexCoord = Vec2(InUvMax.x, InUvMin.y);

    OutVertices.Add(v0); OutVertices.Add(v1); OutVertices.Add(v2); OutVertices.Add(v3);
    OutIndices.Add(base + 0); OutIndices.Add(base + 1); OutIndices.Add(base + 2);
    OutIndices.Add(base + 0); OutIndices.Add(base + 2); OutIndices.Add(base + 3);
}

void UIDrawList::AddRect(const UIRectF& InRectPx, const Vec4& InColor, const Mat4& InTransform) {
    if (InRectPx.Size.x <= 0.0f || InRectPx.Size.y <= 0.0f) {
        return;
    }
    AppendQuad(m_SolidVertices, m_SolidIndices, InRectPx, Vec2(0.0f), Vec2(0.0f), InColor, InTransform);
}

void UIDrawList::AddGlyphQuad(const UIRectF& InRectPx, const Vec2& InUvMin, const Vec2& InUvMax, const Vec4& InColor, const Mat4& InTransform) {
    AppendQuad(m_TextVertices, m_TextIndices, InRectPx, InUvMin, InUvMax, InColor, InTransform);
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

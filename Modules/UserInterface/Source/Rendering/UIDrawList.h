#pragma once
#include "Common/Types.h"
#include "UIVertex.h"
#include "GameFramework/UILayout.h"

class Font;
class Texture;

/** Accumulates a frame of UI geometry into one vertex/index buffer, split into ordered batches
 *  so draw order follows paint order. */
class UIDrawList {
public:
    enum class BatchKind : uint8_t { Solid, Text, Image };

    struct Batch {
        BatchKind Kind;
        uint32_t FirstIndex;
        uint32_t IndexCount;
        Texture* Tex;  // Image batches only; the painting node keeps it alive over the frame
    };

    UIDrawList() = default;

    void AddRect(const UIRectF& InRectPx, const Vec4& InColor, const Mat4& InTransform = Mat4(1.0f));
    void AddGlyphQuad(const UIRectF& InRectPx, const Vec2& InUvMin, const Vec2& InUvMax, const Vec4& InColor, const Mat4& InTransform = Mat4(1.0f));
    void AddText(Font* InFont, const String& InText, const Vec2& InTopLeftPx, float InPixelHeight, const Vec4& InColor, const Mat4& InTransform = Mat4(1.0f));
    /** Null texture falls back to a solid rect. */
    void AddImageRect(const UIRectF& InRectPx, const Vec4& InColor, Texture* InTexture, const Mat4& InTransform = Mat4(1.0f), const Vec2& InUvMin = Vec2(0.0f), const Vec2& InUvMax = Vec2(1.0f));
    void AddImageTriangle(const Vec2 InPoints[3], const Vec2 InUvs[3], const Vec4& InColor, Texture* InTexture, const Mat4& InTransform = Mat4(1.0f));

    /** Solid convex polygon, exactly clipped against the top clip rect. Points must follow the
     *  same winding as AddRect's corners (TL, BL, BR, TR). */
    void AddConvexPolyFilled(const Vec2* InPoints, int32_t InCount, const Vec4& InColor, const Mat4& InTransform = Mat4(1.0f));
    void AddRoundedRect(const UIRectF& InRectPx, const Vec4& InColor, float InRadius, const Mat4& InTransform = Mat4(1.0f));
    /** Rounded rectangle with per-corner radii and a vertical top->bottom color gradient (pass
     *  one color twice for a flat fill). Radii are clamped to the rect's half extent. */
    void AddRoundedRectEx(const UIRectF& InRectPx, const Vec4& InColorTop, const Vec4& InColorBottom,
                          float InRadiusTL, float InRadiusTR, float InRadiusBR, float InRadiusBL, const Mat4& InTransform = Mat4(1.0f));
    void AddLine(const Vec2& InA, const Vec2& InB, float InThickness, const Vec4& InColor, const Mat4& InTransform = Mat4(1.0f));
    /** Cubic bezier stroke; the color blends InColorA -> InColorB along the curve.
     *  InSegments <= 0 picks a count from the curve's size. */
    void AddCubicBezier(const Vec2& InStart, const Vec2& InControlA, const Vec2& InControlB, const Vec2& InEnd,
                        float InThickness, const Vec4& InColorA, const Vec4& InColorB, int32_t InSegments = 0, const Mat4& InTransform = Mat4(1.0f));
    void AddCircle(const Vec2& InCenter, float InRadius, const Vec4& InColor, int32_t InSegments = 16, const Mat4& InTransform = Mat4(1.0f));
    void AddRing(const Vec2& InCenter, float InRadius, float InThickness, const Vec4& InColor, int32_t InSegments = 16, const Mat4& InTransform = Mat4(1.0f));

    void PushClipRect(const UIRectF& InRectPx);
    void PopClipRect();

    const Array<UIVertex>& GetVertices() const { return m_Vertices; }
    const Array<uint32_t>& GetIndices() const { return m_Indices; }
    const Array<Batch>& GetBatches() const { return m_Batches; }
    bool IsEmpty() const { return m_Indices.IsEmpty(); }

private:
    void AppendConvexPoly(const Vec2* InPoints, int32_t InCount, const Vec4& InColorTop, const Vec4& InColorBottom, const Mat4& InTransform);
    void AppendQuad(BatchKind InKind, const UIRectF& InRectPx, const Vec2& InUvMin, const Vec2& InUvMax, const Vec4& InColor, const Mat4& InTransform, Texture* InTexture);
    UIVertex MakeVertex(const Vec2& InPos, const Vec2& InUv, const Vec4& InColor, const Mat4& InTransform) const;
    void ExtendBatch(BatchKind InKind, Texture* InTexture, uint32_t InFirstIndex, uint32_t InIndexCount);

    Array<UIVertex> m_Vertices;
    Array<uint32_t> m_Indices;
    Array<Batch> m_Batches;
    Array<UIRectF> m_ClipStack;
};

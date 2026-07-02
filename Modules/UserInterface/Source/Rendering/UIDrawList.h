#pragma once
#include "Common/Types.h"
#include "Rendering/Vertex.h"
#include "GameFramework/UILayout.h"

class Font;

/** Accumulates a frame of UI geometry in two batches — solid rects and text glyphs — that map to
 *  the two UI pipelines. Geometry is added in pixels and converted to NDC on the fly, so the
 *  produced Vertex arrays upload directly (no projection uniform). Widgets build this via their
 *  Paint() override; you normally only call the Add* helpers. */
class UIDrawList {
public:
    UIDrawList() = default;

    // InTransform maps local pixel geometry into world pixel space (z=0 plane; tilts add depth).
    // Pass a node's world matrix to draw it rotated/tilted; the perspective projection is on the GPU.
    void AddRect(const UIRectF& InRectPx, const Vec4& InColor, const Mat4& InTransform = Mat4(1.0f));
    void AddGlyphQuad(const UIRectF& InRectPx, const Vec2& InUvMin, const Vec2& InUvMax, const Vec4& InColor, const Mat4& InTransform = Mat4(1.0f));
    /** Lay out and append a run of text; InTopLeftPx is the top-left of the text box. */
    void AddText(Font* InFont, const String& InText, const Vec2& InTopLeftPx, float InPixelHeight, const Vec4& InColor, const Mat4& InTransform = Mat4(1.0f));

    const Array<Vertex>& GetSolidVertices() const { return m_SolidVertices; }
    const Array<uint32_t>& GetSolidIndices() const { return m_SolidIndices; }
    const Array<Vertex>& GetTextVertices() const { return m_TextVertices; }
    const Array<uint32_t>& GetTextIndices() const { return m_TextIndices; }

    bool HasSolid() const { return !m_SolidIndices.IsEmpty(); }
    bool HasText() const { return !m_TextIndices.IsEmpty(); }

private:
    void AppendQuad(Array<Vertex>& OutVertices, Array<uint32_t>& OutIndices,
                    const UIRectF& InRectPx, const Vec2& InUvMin, const Vec2& InUvMax, const Vec4& InColor, const Mat4& InTransform);

    Array<Vertex> m_SolidVertices;
    Array<uint32_t> m_SolidIndices;
    Array<Vertex> m_TextVertices;
    Array<uint32_t> m_TextIndices;
};

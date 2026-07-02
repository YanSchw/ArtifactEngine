#pragma once
#include "Common/Types.h"
#include "Rendering/Vertex.h"
#include "GameFramework/UILayout.h"

class Font;

/** Accumulates a frame of UI geometry into a single vertex/index buffer, split into ordered
 *  *batches* (runs of the same kind) so draw order follows the paint (tree) order — a later node
 *  renders in front of an earlier one, even when solids and text interleave. Solid runs draw with
 *  the solid pipeline, Text runs with the SDF-text pipeline; both share this one buffer.
 *
 *  Geometry is added in pixels; the world transform maps it into world pixel space and the GPU does
 *  the perspective projection. Widgets fill this via their Paint() override. */
class UIDrawList {
public:
    enum class BatchKind : uint8_t { Solid, Text };

    struct Batch {
        BatchKind Kind;
        uint32_t FirstIndex;
        uint32_t IndexCount;
    };

    UIDrawList() = default;

    // InTransform maps local pixel geometry into world pixel space (z=0 plane; tilts add depth).
    void AddRect(const UIRectF& InRectPx, const Vec4& InColor, const Mat4& InTransform = Mat4(1.0f));
    void AddGlyphQuad(const UIRectF& InRectPx, const Vec2& InUvMin, const Vec2& InUvMax, const Vec4& InColor, const Mat4& InTransform = Mat4(1.0f));
    /** Lay out and append a run of text; InTopLeftPx is the top-left of the text box. */
    void AddText(Font* InFont, const String& InText, const Vec2& InTopLeftPx, float InPixelHeight, const Vec4& InColor, const Mat4& InTransform = Mat4(1.0f));

    const Array<Vertex>& GetVertices() const { return m_Vertices; }
    const Array<uint32_t>& GetIndices() const { return m_Indices; }
    const Array<Batch>& GetBatches() const { return m_Batches; }
    bool IsEmpty() const { return m_Indices.IsEmpty(); }

private:
    // Appends a quad, extending the current batch if its kind matches or starting a new one.
    void AppendQuad(BatchKind InKind, const UIRectF& InRectPx, const Vec2& InUvMin, const Vec2& InUvMax, const Vec4& InColor, const Mat4& InTransform);

    Array<Vertex> m_Vertices;
    Array<uint32_t> m_Indices;
    Array<Batch> m_Batches;
};

#include "SvgTessellator.h"
#include <cmath>
#include <numbers>

namespace {

void FlattenCubic(const Vec2& InP1, const Vec2& InC1, const Vec2& InC2, const Vec2& InP2,
                  float InToleranceSq, int InDepth, Array<Vec2>& OutPoints) {
    // Flatness: combined distance of both control points from the chord.
    const Vec2 chord = InP2 - InP1;
    const float d1 = std::abs((InC1.x - InP2.x) * chord.y - (InC1.y - InP2.y) * chord.x);
    const float d2 = std::abs((InC2.x - InP2.x) * chord.y - (InC2.y - InP2.y) * chord.x);
    const float chordLengthSq = chord.x * chord.x + chord.y * chord.y;

    if (InDepth >= 10 || (d1 + d2) * (d1 + d2) <= InToleranceSq * chordLengthSq || chordLengthSq < 1e-12f) {
        OutPoints.Add(InP2);
        return;
    }

    const Vec2 p12 = (InP1 + InC1) * 0.5f;
    const Vec2 p23 = (InC1 + InC2) * 0.5f;
    const Vec2 p34 = (InC2 + InP2) * 0.5f;
    const Vec2 p123 = (p12 + p23) * 0.5f;
    const Vec2 p234 = (p23 + p34) * 0.5f;
    const Vec2 mid = (p123 + p234) * 0.5f;
    FlattenCubic(InP1, p12, p123, mid, InToleranceSq, InDepth + 1, OutPoints);
    FlattenCubic(mid, p234, p34, InP2, InToleranceSq, InDepth + 1, OutPoints);
}

/** Contour -> polyline in document space, consecutive duplicates removed. */
Array<Vec2> FlattenContour(const SvgContour& InContour, float InToleranceSq) {
    Array<Vec2> polyline;
    const Array<Vec2>& p = InContour.Points;
    if (p.Size() < 1) {
        return polyline;
    }
    polyline.Add(p[0]);
    for (int32_t i = 0; i + 3 < p.Size(); i += 3) {
        FlattenCubic(p[i], p[i + 1], p[i + 2], p[i + 3], InToleranceSq, 0, polyline);
    }
    Array<Vec2> deduped;
    for (const Vec2& point : polyline) {
        if (deduped.IsEmpty() || glm::distance(deduped.LastItem(), point) > 1e-5f) {
            deduped.Add(point);
        }
    }
    return deduped;
}

struct MeshWriter {
    SvgMesh* Mesh = nullptr;

    uint32_t AddVertex(const Vec2& InPosition, const Vec4& InColor) {
        Mesh->Positions.Add(InPosition);
        Mesh->Colors.Add(InColor);
        return (uint32_t)(Mesh->Positions.Size() - 1);
    }

    /** Emits with the winding the UI renderer expects (negative shoelace area),
     *  flipping if needed; degenerate triangles are dropped. */
    void AddTriangle(const Vec2& InA, const Vec2& InB, const Vec2& InC, const Vec4& InColor) {
        const float signedArea = (InB.x - InA.x) * (InC.y - InA.y) - (InC.x - InA.x) * (InB.y - InA.y);
        if (std::abs(signedArea) < 1e-9f) {
            return;
        }
        const uint32_t a = AddVertex(InA, InColor);
        const uint32_t b = AddVertex(signedArea < 0.0f ? InB : InC, InColor);
        const uint32_t c = AddVertex(signedArea < 0.0f ? InC : InB, InColor);
        Mesh->Indices.Add(a);
        Mesh->Indices.Add(b);
        Mesh->Indices.Add(c);
    }
};

// ---------------------------------------------------------------------------
// Fill: scanline / trapezoid decomposition
// ---------------------------------------------------------------------------

struct FillEdge {
    float X0, Y0;      // top endpoint (smaller y)
    float X1, Y1;      // bottom endpoint
    int Winding;       // +1 when the source edge pointed downward, -1 upward

    float XAt(float InY) const {
        return X0 + (X1 - X0) * ((InY - Y0) / (Y1 - Y0));
    }
};

struct SpanCrossing {
    float XTop, XBottom;
    int Winding;
};

struct FillContext {
    const SvgShape* Shape = nullptr;
    MeshWriter Writer;
    Array<const FillEdge*> ActiveEdges;
    Array<SpanCrossing> Crossings;

    Vec4 ColorAt(const Vec2& InPoint) const {
        return Shape->HasFillGradient ? Shape->FillGradient.Sample(InPoint) : Shape->FillColor;
    }

    void EmitTrapezoid(const SpanCrossing& InLeft, const SpanCrossing& InRight, float InTop, float InBottom) {
        if (InRight.XTop - InLeft.XTop <= 1e-6f && InRight.XBottom - InLeft.XBottom <= 1e-6f) {
            return;
        }
        SvgMesh& mesh = *Writer.Mesh;
        const Vec2 corners[4] = {
            Vec2(InLeft.XTop, InTop), Vec2(InLeft.XBottom, InBottom),
            Vec2(InRight.XBottom, InBottom), Vec2(InRight.XTop, InTop)
        };
        // Corner order mirrors the UI quad winding (TL, BL, BR, TR).
        const uint32_t base = (uint32_t)mesh.Positions.Size();
        for (const Vec2& corner : corners) {
            Writer.AddVertex(corner, ColorAt(corner));
        }
        mesh.Indices.Add(base + 0); mesh.Indices.Add(base + 1); mesh.Indices.Add(base + 2);
        mesh.Indices.Add(base + 0); mesh.Indices.Add(base + 2); mesh.Indices.Add(base + 3);
    }

    /** Emits the spans of one horizontal slab. Edge pairs that cross inside the slab
     *  split it recursively so self-intersecting contours stay exact. */
    void EmitSlab(const Array<FillEdge>& InEdges, float InTop, float InBottom, int InDepth) {
        ActiveEdges.Clear();
        for (const FillEdge& edge : InEdges) {
            if (edge.Y0 <= InTop && edge.Y1 >= InBottom) {
                ActiveEdges.Add(&edge);
            }
        }
        if (ActiveEdges.Size() < 2) {
            return;
        }

        if (InDepth < 6) {
            const float height = InBottom - InTop;
            float splitY = InBottom;
            for (int32_t i = 0; i < ActiveEdges.Size(); i++) {
                for (int32_t j = i + 1; j < ActiveEdges.Size(); j++) {
                    const float topDelta = ActiveEdges[i]->XAt(InTop) - ActiveEdges[j]->XAt(InTop);
                    const float bottomDelta = ActiveEdges[i]->XAt(InBottom) - ActiveEdges[j]->XAt(InBottom);
                    if (topDelta * bottomDelta < -1e-12f) {
                        const float s = topDelta / (topDelta - bottomDelta);
                        const float y = InTop + s * height;
                        if (y > InTop + 1e-5f && y < InBottom - 1e-5f) {
                            splitY = std::min(splitY, y);
                        }
                    }
                }
            }
            if (splitY < InBottom) {
                EmitSlab(InEdges, InTop, splitY, InDepth + 1);
                EmitSlab(InEdges, splitY, InBottom, InDepth + 1);
                return;
            }
        }

        Crossings.Clear();
        for (const FillEdge* edge : ActiveEdges) {
            Crossings.Add({ edge->XAt(InTop), edge->XAt(InBottom), edge->Winding });
        }
        std::sort(Crossings.begin(), Crossings.end(), [](const SpanCrossing& InA, const SpanCrossing& InB) {
            return InA.XTop + InA.XBottom < InB.XTop + InB.XBottom;
        });

        int winding = 0;
        const SpanCrossing* spanStart = nullptr;
        for (const SpanCrossing& crossing : Crossings) {
            const bool wasInside = (Shape->FillRule == SvgFillRule::EvenOdd) ? (winding & 1) : (winding != 0);
            winding += crossing.Winding;
            const bool isInside = (Shape->FillRule == SvgFillRule::EvenOdd) ? (winding & 1) : (winding != 0);
            if (!wasInside && isInside) {
                spanStart = &crossing;
            } else if (wasInside && !isInside && spanStart) {
                EmitTrapezoid(*spanStart, crossing, InTop, InBottom);
                spanStart = nullptr;
            }
        }
    }
};

void FillShape(const SvgShape& InShape, const Array<Array<Vec2>>& InPolygons, SvgMesh& OutMesh) {
    Array<FillEdge> edges;
    Array<float> eventYs;
    for (const Array<Vec2>& polygon : InPolygons) {
        if (polygon.Size() < 3) {
            continue;
        }
        for (int32_t i = 0; i < polygon.Size(); i++) {
            const Vec2& a = polygon[i];
            const Vec2& b = polygon[(i + 1) % polygon.Size()];
            eventYs.Add(a.y);
            if (a.y == b.y) {
                continue;
            }
            if (a.y < b.y) {
                edges.Add({ a.x, a.y, b.x, b.y, +1 });
            } else {
                edges.Add({ b.x, b.y, a.x, a.y, -1 });
            }
        }
    }
    if (edges.IsEmpty()) {
        return;
    }
    std::sort(eventYs.begin(), eventYs.end());

    FillContext context;
    context.Shape = &InShape;
    context.Writer.Mesh = &OutMesh;
    for (int32_t slab = 0; slab + 1 < eventYs.Size(); slab++) {
        if (eventYs[slab + 1] - eventYs[slab] > 1e-9f) {
            context.EmitSlab(edges, eventYs[slab], eventYs[slab + 1], 0);
        }
    }
}

// ---------------------------------------------------------------------------
// Stroke: polyline expansion with joins and caps
// ---------------------------------------------------------------------------

int ArcSegmentCount(float InRadius, float InSweepRadians, float InTolerance) {
    const float maxStep = 2.0f * std::acos(std::clamp(1.0f - InTolerance / std::max(InRadius, 1e-4f), 0.0f, 1.0f));
    const int count = (int)std::ceil(std::abs(InSweepRadians) / std::max(maxStep, 0.1f));
    return std::clamp(count, 1, 64);
}

/** Fan around InCenter from direction InFrom to InTo (unit vectors, sweep < pi). */
void EmitArcFan(MeshWriter& InWriter, const Vec2& InCenter, const Vec2& InFrom, const Vec2& InTo,
                float InRadius, float InTolerance, const Vec4& InColor) {
    const float dot = std::clamp(InFrom.x * InTo.x + InFrom.y * InTo.y, -1.0f, 1.0f);
    float sweep = std::acos(dot);
    if (sweep < 1e-4f) {
        return;
    }
    const float cross = InFrom.x * InTo.y - InFrom.y * InTo.x;
    if (cross < 0.0f) {
        sweep = -sweep;
    }
    const int segments = ArcSegmentCount(InRadius, sweep, InTolerance);
    Vec2 previous = InCenter + InFrom * InRadius;
    for (int i = 1; i <= segments; i++) {
        const float angle = sweep * (float)i / (float)segments;
        const float cosA = std::cos(angle);
        const float sinA = std::sin(angle);
        const Vec2 direction = Vec2(InFrom.x * cosA - InFrom.y * sinA, InFrom.x * sinA + InFrom.y * cosA);
        const Vec2 current = InCenter + direction * InRadius;
        InWriter.AddTriangle(InCenter, previous, current, InColor);
        previous = current;
    }
}

void EmitJoin(MeshWriter& InWriter, const SvgShape& InShape, const Vec2& InVertex,
              const Vec2& InDirIn, const Vec2& InDirOut, float InHalfWidth, float InTolerance) {
    const float cross = InDirIn.x * InDirOut.y - InDirIn.y * InDirOut.x;
    if (std::abs(cross) < 1e-6f) {
        return; // collinear, segment quads already meet
    }
    // Unit normals of both segments on the turn's outer side.
    const float side = cross > 0.0f ? -1.0f : 1.0f;
    const Vec2 normalIn = Vec2(-InDirIn.y, InDirIn.x) * side;
    const Vec2 normalOut = Vec2(-InDirOut.y, InDirOut.x) * side;

    if (InShape.Join == SvgLineJoin::Round) {
        EmitArcFan(InWriter, InVertex, normalIn, normalOut, InHalfWidth, InTolerance, InShape.StrokeColor);
        return;
    }

    const Vec2 outerIn = InVertex + normalIn * InHalfWidth;
    const Vec2 outerOut = InVertex + normalOut * InHalfWidth;
    if (InShape.Join == SvgLineJoin::Miter) {
        Vec2 miterDirection = normalIn + normalOut;
        const float length = glm::length(miterDirection);
        if (length > 1e-6f) {
            miterDirection /= length;
            const float cosHalfAngle = miterDirection.x * normalIn.x + miterDirection.y * normalIn.y;
            if (cosHalfAngle > 1e-4f && 1.0f / cosHalfAngle <= InShape.MiterLimit) {
                const Vec2 miterPoint = InVertex + miterDirection * (InHalfWidth / cosHalfAngle);
                InWriter.AddTriangle(InVertex, outerIn, miterPoint, InShape.StrokeColor);
                InWriter.AddTriangle(InVertex, miterPoint, outerOut, InShape.StrokeColor);
                return;
            }
        }
        // Falls through to a bevel when past the miter limit.
    }
    InWriter.AddTriangle(InVertex, outerIn, outerOut, InShape.StrokeColor);
}

void EmitCap(MeshWriter& InWriter, const SvgShape& InShape, const Vec2& InEnd,
             const Vec2& InOutwardDir, float InHalfWidth, float InTolerance) {
    const Vec2 normal = Vec2(-InOutwardDir.y, InOutwardDir.x);
    if (InShape.Cap == SvgLineCap::Square) {
        const Vec2 extended = InEnd + InOutwardDir * InHalfWidth;
        InWriter.AddTriangle(InEnd - normal * InHalfWidth, InEnd + normal * InHalfWidth, extended + normal * InHalfWidth, InShape.StrokeColor);
        InWriter.AddTriangle(InEnd - normal * InHalfWidth, extended + normal * InHalfWidth, extended - normal * InHalfWidth, InShape.StrokeColor);
    } else if (InShape.Cap == SvgLineCap::Round) {
        EmitArcFan(InWriter, InEnd, normal, InOutwardDir, InHalfWidth, InTolerance, InShape.StrokeColor);
        EmitArcFan(InWriter, InEnd, InOutwardDir, -normal, InHalfWidth, InTolerance, InShape.StrokeColor);
    }
}

void StrokeShape(const SvgShape& InShape, const Array<Array<Vec2>>& InPolylines,
                 const Array<uint8_t>& InClosedFlags, float InTolerance, SvgMesh& OutMesh) {
    MeshWriter writer;
    writer.Mesh = &OutMesh;
    const float halfWidth = InShape.StrokeWidth * 0.5f;

    for (int32_t contourIndex = 0; contourIndex < InPolylines.Size(); contourIndex++) {
        Array<Vec2> points = InPolylines[contourIndex];
        const bool closed = InClosedFlags[contourIndex] != 0;
        if (closed && points.Size() >= 2 && glm::distance(points[0], points.LastItem()) < 1e-5f) {
            points.RemoveLastItem();
        }
        if (points.Size() < 2) {
            continue;
        }
        const int32_t segmentCount = closed ? points.Size() : points.Size() - 1;

        for (int32_t i = 0; i < segmentCount; i++) {
            const Vec2& a = points[i];
            const Vec2& b = points[(i + 1) % points.Size()];
            const Vec2 direction = glm::normalize(b - a);
            const Vec2 normal = Vec2(-direction.y, direction.x) * halfWidth;
            writer.AddTriangle(a - normal, a + normal, b + normal, InShape.StrokeColor);
            writer.AddTriangle(a - normal, b + normal, b - normal, InShape.StrokeColor);
        }

        const int32_t joinCount = closed ? points.Size() : points.Size() - 2;
        for (int32_t j = 0; j < joinCount; j++) {
            const int32_t vertex = closed ? j : j + 1;
            const Vec2& previous = points[(vertex + points.Size() - 1) % points.Size()];
            const Vec2& current = points[vertex];
            const Vec2& next = points[(vertex + 1) % points.Size()];
            EmitJoin(writer, InShape, current, glm::normalize(current - previous), glm::normalize(next - current), halfWidth, InTolerance);
        }

        if (!closed) {
            EmitCap(writer, InShape, points[0], glm::normalize(points[0] - points[1]), halfWidth, InTolerance);
            EmitCap(writer, InShape, points.LastItem(),
                    glm::normalize(points.LastItem() - points[points.Size() - 2]), halfWidth, InTolerance);
        }
    }
}

} // namespace

void SvgTessellator::Tessellate(const SvgDocument& InDocument, float InScale, SvgMesh& OutMesh) {
    OutMesh.Positions.Clear();
    OutMesh.Colors.Clear();
    OutMesh.Indices.Clear();

    const float scale = std::max(InScale, 0.01f);
    const float tolerance = 0.35f / scale;  // ~1/3 pixel of curve error at the target scale

    for (const SvgShape& shape : InDocument.Shapes) {
        Array<Array<Vec2>> polylines;
        Array<uint8_t> closedFlags;
        for (const SvgContour& contour : shape.Contours) {
            Array<Vec2> polyline = FlattenContour(contour, tolerance * tolerance);
            if (!polyline.IsEmpty()) {
                polylines.Add(std::move(polyline));
                closedFlags.Add(contour.Closed ? 1 : 0);
            }
        }

        if (shape.HasFill) {
            FillShape(shape, polylines, OutMesh);
        }
        if (shape.HasStroke) {
            StrokeShape(shape, polylines, closedFlags, tolerance, OutMesh);
        }
    }
}

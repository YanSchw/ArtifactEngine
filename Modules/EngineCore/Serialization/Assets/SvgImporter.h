#pragma once
#include "Common/Types.h"
#include "Common/Array.h"

/** One subpath in document space. Points holds the first on-curve point followed by
 *  cubic bezier triplets (control1, control2, end), i.e. Size() == 1 + 3n. */
struct SvgContour {
    Array<Vec2> Points;
    bool Closed = false;
};

enum class SvgFillRule : uint8_t { NonZero = 0, EvenOdd = 1 };
enum class SvgLineCap : uint8_t { Butt = 0, Round = 1, Square = 2 };
enum class SvgLineJoin : uint8_t { Miter = 0, Round = 1, Bevel = 2 };

struct SvgGradientStop {
    float Offset = 0.0f;
    Vec4 Color = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
};

/** Linear gradient resolved into document space: T(p) = dot(TCoeffs, (p.x, p.y, 1))
 *  yields the clamped stop parameter for any document-space point. */
struct SvgLinearGradient {
    Vec3 TCoeffs = Vec3(0.0f);
    Array<SvgGradientStop> Stops;

    Vec4 Sample(const Vec2& InPoint) const;
};

struct SvgShape {
    bool HasFill = true;
    Vec4 FillColor = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    SvgFillRule FillRule = SvgFillRule::NonZero;
    /* When set, FillColor holds the gradient's average as a flat fallback. */
    bool HasFillGradient = false;
    SvgLinearGradient FillGradient;

    bool HasStroke = false;
    Vec4 StrokeColor = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    float StrokeWidth = 1.0f;
    SvgLineCap Cap = SvgLineCap::Butt;
    SvgLineJoin Join = SvgLineJoin::Miter;
    float MiterLimit = 4.0f;

    Array<SvgContour> Contours;
};

struct SvgDocument {
    float Width = 0.0f;
    float Height = 0.0f;
    Array<SvgShape> Shapes;
};

/** Parses the drawable geometry of an SVG, not the full spec. */
namespace SvgImporter {
    bool Parse(const String& InSvgText, SvgDocument& OutDocument);
}

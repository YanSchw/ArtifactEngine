#pragma once
#include "Common/Types.h"

/** A layout scalar: a fraction of the parent's content extent plus a pixel offset, resolved as
 *  `Fraction * parent + Pixels` (CSS calc-style). Plain floats convert implicitly as pixels, so
 *  `Size = Vec2(160, 40)` keeps meaning pixels; the `_px` / `_rel` literals compose with +/- :
 *      node->Size = { 1.0_rel - 40.0_px, 22.0_px };   // parent width minus 40px, 22px tall */
struct UIValue {
    float Fraction = 0.0f;
    float Pixels = 0.0f;

    UIValue() = default;
    UIValue(float InPixels) : Pixels(InPixels) { }
    UIValue(float InFraction, float InPixels) : Fraction(InFraction), Pixels(InPixels) { }

    static UIValue Px(float InPixels) { return UIValue(0.0f, InPixels); }
    static UIValue Rel(float InFraction) { return UIValue(InFraction, 0.0f); }

    /** This value in pixels, given the parent's extent along the same axis. */
    float Resolve(float InParentExtent) const { return Fraction * InParentExtent + Pixels; }

    UIValue operator+(const UIValue& InOther) const { return UIValue(Fraction + InOther.Fraction, Pixels + InOther.Pixels); }
    UIValue operator-(const UIValue& InOther) const { return UIValue(Fraction - InOther.Fraction, Pixels - InOther.Pixels); }
    UIValue operator-() const { return UIValue(-Fraction, -Pixels); }
};

inline UIValue operator""_px(long double InPixels) { return UIValue::Px((float)InPixels); }
inline UIValue operator""_px(unsigned long long InPixels) { return UIValue::Px((float)InPixels); }
inline UIValue operator""_rel(long double InFraction) { return UIValue::Rel((float)InFraction); }
inline UIValue operator""_rel(unsigned long long InFraction) { return UIValue::Rel((float)InFraction); }

/** A 2D layout vector whose X and Y are each a UIValue — the type behind Position and Size.
 *  Mixing units per axis is the point: `{ 1.0_rel, 22.0_px }`. Converts implicitly from Vec2
 *  (both axes pixels). */
struct UIVec2 {
    UIValue X;
    UIValue Y;

    UIVec2() = default;
    UIVec2(const UIValue& InX, const UIValue& InY) : X(InX), Y(InY) { }
    UIVec2(const Vec2& InPixels) : X(InPixels.x), Y(InPixels.y) { }

    /** Both axes in pixels, given the parent's content size. */
    Vec2 Resolve(const Vec2& InParentSize) const {
        return Vec2(X.Resolve(InParentSize.x), Y.Resolve(InParentSize.y));
    }
};

/** Per-edge insets shrinking the rect a node's children lay out in. */
struct UIPadding {
    UIValue Left, Top, Right, Bottom;

    UIPadding() = default;
    UIPadding(const UIValue& InAll) : Left(InAll), Top(InAll), Right(InAll), Bottom(InAll) { }
    UIPadding(const UIValue& InHorizontal, const UIValue& InVertical)
        : Left(InHorizontal), Top(InVertical), Right(InHorizontal), Bottom(InVertical) { }
    UIPadding(const UIValue& InLeft, const UIValue& InTop, const UIValue& InRight, const UIValue& InBottom)
        : Left(InLeft), Top(InTop), Right(InRight), Bottom(InBottom) { }
};

/** A rectangle in screen pixels, top-left origin (+X right, +Y down). Position is the top-left. */
struct UIRectF {
    Vec2 Position = Vec2(0.0f);
    Vec2 Size = Vec2(0.0f);

    UIRectF() = default;
    UIRectF(const Vec2& InPosition, const Vec2& InSize) : Position(InPosition), Size(InSize) { }
    UIRectF(float InX, float InY, float InWidth, float InHeight) : Position(InX, InY), Size(InWidth, InHeight) { }

    Vec2 Min() const { return Position; }
    Vec2 Max() const { return Position + Size; }
    Vec2 Center() const { return Position + Size * 0.5f; }

    bool Contains(const Vec2& InPoint) const {
        return InPoint.x >= Position.x && InPoint.x <= Position.x + Size.x
            && InPoint.y >= Position.y && InPoint.y <= Position.y + Size.y;
    }

    /** This rect shrunk inward by the given padding */
    UIRectF Deflate(const UIPadding& InPadding) const {
        const float left = InPadding.Left.Resolve(Size.x);
        const float right = InPadding.Right.Resolve(Size.x);
        const float top = InPadding.Top.Resolve(Size.y);
        const float bottom = InPadding.Bottom.Resolve(Size.y);
        return UIRectF(
            Position + Vec2(left, top),
            Vec2(std::max(0.0f, Size.x - left - right), std::max(0.0f, Size.y - top - bottom))
        );
    }
};

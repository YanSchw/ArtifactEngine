#pragma once
#include "Common/Types.h"

/** Per-edge insets in pixels — used for CSS-style margin (outer) and padding (inner). */
struct UIEdges {
    float Left = 0.0f, Top = 0.0f, Right = 0.0f, Bottom = 0.0f;

    UIEdges() = default;
    UIEdges(float InAll) : Left(InAll), Top(InAll), Right(InAll), Bottom(InAll) { }
    UIEdges(float InHorizontal, float InVertical)
        : Left(InHorizontal), Top(InVertical), Right(InHorizontal), Bottom(InVertical) { }
    UIEdges(float InLeft, float InTop, float InRight, float InBottom)
        : Left(InLeft), Top(InTop), Right(InRight), Bottom(InBottom) { }

    float Horizontal() const { return Left + Right; }
    float Vertical() const { return Top + Bottom; }
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

    /** This rect shrunk inward by the given edges (never negative size). */
    UIRectF Deflate(const UIEdges& InEdges) const {
        return UIRectF(
            Position + Vec2(InEdges.Left, InEdges.Top),
            Vec2(std::max(0.0f, Size.x - InEdges.Horizontal()), std::max(0.0f, Size.y - InEdges.Vertical()))
        );
    }
};

/** How a node arranges its children: None = each child via its own anchors; SplitX/SplitY =
 *  distribute the content rect as a row / column. */
enum class UILayoutMode : uint8_t { None = 0, SplitX = 1, SplitY = 2 };

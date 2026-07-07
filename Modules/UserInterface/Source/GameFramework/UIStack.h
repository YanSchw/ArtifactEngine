#pragma once
#include "UINode.h"
#include "UIStack.gen.h"

/** Axis a UIStack arranges its children along. */
enum class UIAxis : uint8_t { X = 0, Y = 1 };

/** Arranges its children in a row (Axis::X) or column (Axis::Y), separated by Gap.
 *
 *  Each child's slot along the stack axis comes from the child's own Size on that axis:
 *  the Pixels part is a fixed extent, and the Fraction part is a flex weight over the space
 *  left after all fixed parts and gaps (share = Fraction / max(sum of fractions, 1) — so
 *  all-Fill() children split the stack evenly, and fixed-size children pack in order):
 *
 *      UI::VStack(canvas, [](UIStack& v) {
 *          v.Gap = 2.0_px;
 *          UI::Button(v, "A", ...).Size = { 1.0_rel, 25.0_px };  // fixed 25px rows...
 *          UI::Button(v, "B", ...).Size = { 1.0_rel, 25.0_px };
 *          v.Add<UINode>()->Size = { 0.0_px, 1.0_rel };          // flexible spacer
 *          UI::Button(v, "C", ...).Size = { 1.0_rel, 25.0_px };  // ...pushed to the bottom
 *      });
 *
 *  Each child then lays out inside its slot with the normal rect model, so the cross axis is
 *  the child's own business (1.0_rel spans the slot). When the fixed extents + gaps exceed the
 *  content rect the stack overflows it — children keep their stated sizes and march past the
 *  end (negative sizes/weights are treated as 0). */
class UIStack : public UINode {
public:
    ARTIFACT_CLASS();

    UIAxis Axis = UIAxis::Y;
    UIValue Gap = 0.0f;

protected:
    virtual void LayoutChildren(const UIRectF& InContent) override;
};

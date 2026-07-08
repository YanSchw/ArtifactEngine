#include "UIStack.h"
#include <algorithm>

void UIStack::LayoutChildren(const UIRectF& InContent) {
    Array<UINode*> children = CollectUIChildren();
    if (children.IsEmpty()) {
        return;
    }

    const bool horizontal = (Axis == UIAxis::X);
    const float contentExtent = horizontal ? InContent.Size.x : InContent.Size.y;
    const float gap = std::max(0.0f, Gap.Resolve(contentExtent));

    float fixedTotal = 0.0f;
    float weightSum = 0.0f;
    for (UINode* child : children) {
        const UIValue& main = horizontal ? child->Size.X : child->Size.Y;
        fixedTotal += std::max(0.0f, main.Pixels);
        weightSum += std::max(0.0f, main.Fraction);
    }

    const float gapTotal = gap * (float)(children.Size() - 1);
    const float leftover = std::max(0.0f, contentExtent - fixedTotal - gapTotal);
    const float weightNorm = std::max(weightSum, 1.0f);

    float cursor = horizontal ? InContent.Position.x : InContent.Position.y;
    for (UINode* child : children) {
        const UIValue& main = horizontal ? child->Size.X : child->Size.Y;
        const float extent = std::max(0.0f, main.Pixels) + std::max(0.0f, main.Fraction) / weightNorm * leftover;
        const UIRectF slot = horizontal
            ? UIRectF(Vec2(cursor, InContent.Position.y), Vec2(extent, InContent.Size.y))
            : UIRectF(Vec2(InContent.Position.x, cursor), Vec2(InContent.Size.x, extent));
        LayoutChild(*child, slot, m_WorldMatrix);
        cursor += extent + gap;
    }
}

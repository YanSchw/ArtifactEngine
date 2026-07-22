#include "UIGrid.h"
#include <algorithm>
#include <cmath>

void UIGrid::LayoutChildren(const UIRectF& InContent) {
    const float stepX = CellSize.x + Gap.x;
    const float stepY = CellSize.y + Gap.y;
    const int columns = std::max(1, (int)std::floor((InContent.Size.x + Gap.x) / stepX));

    int index = 0;
    for (UINode* child : CollectUIChildren()) {
        if (!child->IsEnabled()) {
            continue;
        }
        const int column = index % columns;
        const int row = index / columns;
        const Vec2 position = InContent.Position + Vec2((float)column * stepX, (float)row * stepY);

        // The grid owns each child's outer rect
        child->Anchor = child->Pivot = Vec2(0.0f);
        child->Position = Vec2(0.0f);
        child->Size = { 1.0_rel, 1.0_rel };
        LayoutChild(*child, UIRectF(position, CellSize), m_WorldMatrix);

        index++;
    }
}

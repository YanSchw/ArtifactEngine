#include "DetailsItem.h"
#include "UI/EditorStyle.h"
#include "UI/UITooltip.h"
#include "Rendering/UIDrawList.h"

void DetailsItem::SetTooltip(const String& InText) {
    if (InText.empty()) {
        return;
    }
    UITooltip* tooltip = Add<UITooltip>();
    tooltip->Text = InText;
    tooltip->GetAnchor = [this] { return GetTooltipAnchor(); };
}

void DetailsItem::PaintIndentGutter(UIDrawList& OutDrawList, int32_t InLevels, const UIRectF& InRect) const {
    if (InLevels <= 0) {
        return;
    }
    OutDrawList.AddRect(UIRectF(InRect.Min(), Vec2(IndentWidth * (float)InLevels, InRect.Size.y)), EditorStyle::PanelDark, m_WorldMatrix);
    for (int32_t level = 1; level <= InLevels; level++) {
        const UIRectF divider(Vec2(InRect.Min().x + IndentWidth * (float)level - 1.0f, InRect.Min().y), Vec2(1.0f, InRect.Size.y));
        OutDrawList.AddRect(divider, EditorStyle::Border, m_WorldMatrix);
    }
}

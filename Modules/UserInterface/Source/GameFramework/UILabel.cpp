#include "UILabel.h"
#include "Rendering/UIDrawList.h"
#include "Assets/Font.h"

void UILabel::Paint(UIDrawList& OutDrawList) {
    UINode::Paint(OutDrawList); // optional background

    Font* font = FontAsset ? FontAsset : GetDefaultFont();
    if (!font || Text.empty()) {
        return;
    }

    const Vec2 textSize = font->MeasureText(Text, FontSize);
    const UIRectF content = GetContentRect();

    float x = content.Position.x;
    if (HAlign == UIHAlign::Center) x = content.Position.x + (content.Size.x - textSize.x) * 0.5f;
    else if (HAlign == UIHAlign::Right) x = content.Max().x - textSize.x;

    float y = content.Position.y;
    if (VAlign == UIVAlign::Middle) y = content.Position.y + (content.Size.y - textSize.y) * 0.5f;
    else if (VAlign == UIVAlign::Bottom) y = content.Max().y - textSize.y;

    OutDrawList.AddText(font, Text, Vec2(x, y), FontSize, Color, m_WorldMatrix);
}

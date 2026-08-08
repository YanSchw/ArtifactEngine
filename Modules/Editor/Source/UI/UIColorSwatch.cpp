#include "UIColorSwatch.h"

#include "ColorPickerWindow.h"
#include "EditorStyle.h"
#include "Rendering/UIDrawList.h"

static constexpr float s_CheckerSize = 6.0f;
static const Vec4 s_CheckerLight = HexColor(0x9A9A9A);
static const Vec4 s_CheckerDark = HexColor(0x6E6E6E);

UIColorSwatch::UIColorSwatch() {
    Interactable = true;
    Cursor = CursorIcon::Hand;
    Size = Vec2(60.0f, 18.0f);
}

void UIColorSwatch::PaintColor(UIDrawList& OutDrawList, const UIRectF& InRect, const Color& InColor,
                               float InRadius, const Mat4& InTransform) {
    if (InColor.a < 1.0f) {
        OutDrawList.PushClipRect(InRect);
        for (float y = InRect.Min().y; y < InRect.Max().y; y += s_CheckerSize) {
            for (float x = InRect.Min().x; x < InRect.Max().x; x += s_CheckerSize) {
                const int32_t cell = (int32_t)((x - InRect.Min().x) / s_CheckerSize)
                                   + (int32_t)((y - InRect.Min().y) / s_CheckerSize);
                OutDrawList.AddRect(UIRectF(Vec2(x, y), Vec2(s_CheckerSize)),
                                    cell % 2 == 0 ? s_CheckerLight : s_CheckerDark, InTransform);
            }
        }
        OutDrawList.PopClipRect();
    }
    OutDrawList.AddRoundedRect(InRect, InColor, InRadius, InTransform);
}

void UIColorSwatch::Paint(UIDrawList& OutDrawList) {
    Color color = Get ? Get() : Color(0.0f);
    if (!UseAlpha) {
        color.a = 1.0f;
    }

    const Vec4 border = IsHovered() ? EditorStyle::Accent : EditorStyle::FieldBorder;
    OutDrawList.AddRoundedRect(m_Geometry, border, CornerRadius, m_WorldMatrix);
    PaintColor(OutDrawList, m_Geometry.Deflate(UIPadding(1.0f)), color, CornerRadius - 1.0f, m_WorldMatrix);
}

void UIColorSwatch::OnClick() {
    if (!Get || !Set) {
        return;
    }
    ColorPickerWindow::Open(Get(), UseAlpha, Title, LocalToScreen(m_Geometry.Max()), Set);
}

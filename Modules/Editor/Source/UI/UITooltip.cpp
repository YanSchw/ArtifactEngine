#include "UITooltip.h"
#include "EditorStyle.h"
#include "GameFramework/UICanvas.h"
#include "Assets/Font.h"
#include "Rendering/UIDrawList.h"
#include <algorithm>

static constexpr float s_Delay = 0.6f;
static constexpr float s_Margin = 6.0f;
static const Vec2 s_Padding = Vec2(8.0f, 5.0f);

UITooltipLayer::UITooltipLayer() {
    Fill();
    Interactable = false;
}

UITooltipLayer* UITooltipLayer::EnsureFor(UINode& InOwner) {
    UICanvas* canvas = InOwner.GetCanvas();
    if (!canvas) {
        return nullptr;
    }
    if (Node* existing = canvas->GetChildByClass(UITooltipLayer::StaticClass())) {
        return existing->As<UITooltipLayer>();
    }
    return canvas->Add<UITooltipLayer>();
}

void UITooltipLayer::Show(const String& InText, const UIRectF& InAnchor) {
    m_Text = InText;
    m_Anchor = InAnchor;
}

void UITooltipLayer::PaintOverlay(UIDrawList& OutDrawList) {
    Font* font = GetDefaultFont();
    if (m_Text.empty() || !font) {
        return;
    }

    const Vec2 size = font->MeasureText(m_Text, EditorStyle::FontSize) + s_Padding * 2.0f;
    Vec2 position(m_Anchor.Min().x, m_Anchor.Max().y + s_Margin);
    if (position.y + size.y > m_Geometry.Max().y - s_Margin) {
        position.y = m_Anchor.Min().y - size.y - s_Margin;
    }
    position.x = std::min(position.x, m_Geometry.Max().x - s_Margin - size.x);
    position = Vec2(std::max(position.x, m_Geometry.Min().x + s_Margin),
                    std::max(position.y, m_Geometry.Min().y + s_Margin));

    const UIRectF rect(position, size);
    OutDrawList.AddRoundedRect(rect, EditorStyle::Border, 4.0f);
    OutDrawList.AddRoundedRect(rect.Deflate(UIPadding(1.0f)), EditorStyle::ToolBar, 3.0f);
    OutDrawList.AddText(font, m_Text, position + s_Padding, EditorStyle::FontSize, EditorStyle::Text);

    m_Text.clear();  // the hovered node asks for it again in the next update pass
}

UITooltip::UITooltip() {
    Fill();
    Interactable = false;
}

void UITooltip::OnUIUpdate(const UIFrameContext& InContext) {
    UINode* host = GetParent() ? GetParent()->As<UINode>() : nullptr;
    if (!host || !host->IsHovered() || InContext.CursorDown || Text.empty()) {
        m_HoverSeconds = 0.0f;
        return;
    }

    m_HoverSeconds += InContext.DeltaTime;
    if (m_HoverSeconds < s_Delay) {
        return;
    }
    if (!m_Layer.Get()) {
        m_Layer = UITooltipLayer::EnsureFor(*this);
    }
    if (UITooltipLayer* layer = m_Layer.Get()) {
        layer->Show(Text, host->GetGeometry());
    }
}

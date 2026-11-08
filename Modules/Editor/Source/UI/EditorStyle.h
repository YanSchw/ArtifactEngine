#pragma once
#include "Common/Types.h"
#include "GameFramework/UIButton.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UISvg.h"
#include "Assets/Font.h"
#include <algorithm>
#include "UITooltip.h"

inline Vec4 HexColor(uint32_t InRGB, float InAlpha = 1.0f) {
    return Vec4(
        (float)((InRGB >> 16) & 0xFF) / 255.0f,
        (float)((InRGB >> 8) & 0xFF) / 255.0f,
        (float)(InRGB & 0xFF) / 255.0f,
        InAlpha);
}

struct EditorAppearanceSettings;

/** The editor's visual language: a dark color palette plus the fixed chrome metrics.
 *  Every editor UI node pulls its colors from here so the theme stays in one place. The palette
 *  and font size are set once by Apply(), before any chrome is built. */
class EditorStyle {
public:
    static void Apply(const EditorAppearanceSettings& InAppearance);

    // Chrome
    inline static Vec4 TitleBar = HexColor(0x151515);
    inline static Vec4 ToolBar = HexColor(0x1D1D1D);
    inline static Vec4 BottomBar = HexColor(0x181818);
    inline static Vec4 Border = HexColor(0x0C0C0C);

    // Panels
    inline static Vec4 Panel = HexColor(0x242424);
    inline static Vec4 PanelDark = HexColor(0x161616);
    inline static const Vec4 Folder = HexColor(0xD1A24C);
    inline static Vec4 TabBar = HexColor(0x181818);
    inline static Vec4 TabActive = HexColor(0x242424);
    inline static Vec4 TabHover = HexColor(0x2D2D2D);

    // Text
    inline static Vec4 Text = HexColor(0xC8C8C8);
    inline static Vec4 TextDim = HexColor(0x8F8F8F);
    inline static Vec4 TextBright = HexColor(0xFFFFFF);

    // Interaction
    inline static Vec4 Accent = HexColor(0x0070E0);
    inline static Vec4 AccentBright = HexColor(0x26BBFF);
    inline static Vec4 Button = HexColor(0x2E2E2E);
    inline static Vec4 ButtonHover = HexColor(0x3A3A3A);
    inline static Vec4 ButtonPressed = HexColor(0x232323);
    inline static Vec4 FieldBorder = HexColor(0x3A3A3A);
    inline static Vec4 ButtonBorder = HexColor(0x3C3C3C);
    inline static const Vec4 CaptionHover = Vec4(1.0f, 1.0f, 1.0f, 0.07f);
    inline static const Vec4 CaptionCloseHover = HexColor(0xC42B1C);
    inline static const Vec4 DropPreview = HexColor(0x26BBFF, 0.28f);

    inline static const Vec4 TransformX = HexColor(0xD84A4A);
    inline static const Vec4 TransformY = HexColor(0x71B33C);
    inline static const Vec4 TransformZ = HexColor(0x3D7BD8);

    // Shortcuts
#if defined(AE_PLATFORM_MACOS)
    inline static const String CommandKey = "Cmd";
#else
    inline static const String CommandKey = "Ctrl";
#endif

    // Metrics
    inline static constexpr float TitleBarHeight = 36.0f;
    inline static constexpr float TitleLogoHeight = 13.0f;
    inline static constexpr float ToolBarHeight = 40.0f;
    inline static constexpr float BottomBarHeight = 34.0f;
    inline static constexpr float TabHeaderHeight = 26.0f;
    inline static constexpr float CaptionButtonWidth = 46.0f;
    inline static constexpr float ToolButtonWidth = 30.0f;
    inline static constexpr float ToolIconSize = 16.0f;
    inline static constexpr float ButtonRadius = 4.0f;
    inline static constexpr float SplitterThickness = 4.0f;
    inline static constexpr float MinPanelSize = 90.0f;
    inline static float FontSize = 13.0f;
    inline static constexpr float MacTrafficLightInset = 76.0f;

    static void StyleButton(UIButton& InButton) {
        InButton.NormalColor = Button;
        InButton.HoverColor = ButtonHover;
        InButton.PressedColor = ButtonPressed;
        InButton.BorderColor = ButtonBorder;
        InButton.CornerRadius = Vec4(ButtonRadius);
    }

    static void SetToggled(UIButton& InButton, bool InToggled) {
        const Vec4 accentHover = Vec4(glm::mix(Vec3(Accent), Vec3(1.0f), 0.15f), 1.0f);
        InButton.NormalColor = InToggled ? Accent : Button;
        InButton.HoverColor = InToggled ? accentHover : ButtonHover;
        InButton.BorderColor = InToggled ? accentHover : ButtonBorder;
    }

    /** Rounds only the outer corners, so adjacent buttons read as one control. */
    static void StyleSegment(UIButton& InButton, bool InFirst, bool InLast) {
        StyleButton(InButton);
        InButton.CornerRadius = Vec4(InFirst ? ButtonRadius : 0.0f, InLast ? ButtonRadius : 0.0f,
                                     InLast ? ButtonRadius : 0.0f, InFirst ? ButtonRadius : 0.0f);
    }

    static void ApplyButtonStyle(UIButton& InButton, float InFontSize = FontSize) {
        StyleButton(InButton);
        if (Node* child = InButton.GetChildByClass(UILabel::StaticClass())) {
            UILabel* label = child->As<UILabel>();
            label->FontSize = InFontSize;
            label->Color = Text;
        }
    }

    /** An icon-only button with a tooltip. */
    static UIButton& ToolButton(UINode& InParent, VectorImage* InIcon, const String& InTooltip,
                                std::function<void()> InAction, float InWidth = ToolButtonWidth) {
        UIButton* button = InParent.Add<UIButton>();
        button->Clicked = std::move(InAction);
        button->Size = { UIValue(InWidth), 1.0_rel };
        StyleButton(*button);

        UISvg* icon = button->Add<UISvg>();
        icon->Center(Vec2(ToolIconSize));
        icon->Tint = Text;
        icon->Image = InIcon;

        button->Add<UITooltip>()->Text = InTooltip;
        return *button;
    }

    /** Icon and caption centered as one group; InMinWidth only ever widens the button. */
    static UIButton& IconButton(UINode& InParent, VectorImage* InIcon, const Vec4& InIconTint,
                                const String& InCaption, float InMinWidth, std::function<void()> InAction) {
        constexpr float iconSize = 14.0f;
        constexpr float iconGap = 6.0f;
        constexpr float padding = 12.0f;
        Font* font = UINode::GetDefaultFont();
        const float textWidth = font ? font->GetTextWidth(InCaption, FontSize) : 8.0f * (float)InCaption.size();
        const float contentWidth = iconSize + iconGap + textWidth;
        const float width = std::max(InMinWidth, contentWidth + 2.0f * padding);
        const float left = (width - contentWidth) * 0.5f;

        UIButton* button = InParent.Add<UIButton>();
        button->Clicked = std::move(InAction);
        button->Size = { UIValue(width), 1.0_rel };
        StyleButton(*button);

        UISvg* icon = button->Add<UISvg>();
        icon->Anchor = icon->Pivot = Vec2(0.0f, 0.5f);
        icon->Position = Vec2(left, 0.0f);
        icon->Size = Vec2(iconSize);
        icon->Tint = InIconTint;
        icon->Image = InIcon;

        UILabel* label = button->Add<UILabel>();
        label->Anchor = label->Pivot = Vec2(0.0f, 0.5f);
        label->Position = Vec2(left + iconSize + iconGap, 0.0f);
        label->Size = { UIValue(textWidth + 4.0f), 1.0_rel };
        label->FontSize = FontSize;
        label->Color = Text;
        label->VAlign = UIVAlign::Middle;
        label->Text = InCaption;
        return *button;
    }
};
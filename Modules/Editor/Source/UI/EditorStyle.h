#pragma once
#include "Common/Types.h"
#include "GameFramework/UIButton.h"
#include "GameFramework/UILabel.h"

inline Vec4 HexColor(uint32_t InRGB, float InAlpha = 1.0f) {
    return Vec4(
        (float)((InRGB >> 16) & 0xFF) / 255.0f,
        (float)((InRGB >> 8) & 0xFF) / 255.0f,
        (float)(InRGB & 0xFF) / 255.0f,
        InAlpha);
}

/** The editor's visual language: a dark color palette plus the fixed chrome metrics.
 *  Every editor UI node pulls its colors from here so the theme stays in one place. */
class EditorStyle {
public:
    // Chrome
    inline static const Vec4 TitleBar = HexColor(0x151515);
    inline static const Vec4 ToolBar = HexColor(0x1D1D1D);
    inline static const Vec4 BottomBar = HexColor(0x181818);
    inline static const Vec4 Border = HexColor(0x0C0C0C);

    // Panels
    inline static const Vec4 Panel = HexColor(0x242424);
    inline static const Vec4 PanelDark = HexColor(0x161616);
    inline static const Vec4 Folder = HexColor(0xD1A24C);
    inline static const Vec4 TabBar = HexColor(0x181818);
    inline static const Vec4 TabActive = HexColor(0x242424);
    inline static const Vec4 TabHover = HexColor(0x2D2D2D);

    // Text
    inline static const Vec4 Text = HexColor(0xC8C8C8);
    inline static const Vec4 TextDim = HexColor(0x8F8F8F);
    inline static const Vec4 TextBright = HexColor(0xFFFFFF);

    // Interaction
    inline static const Vec4 Accent = HexColor(0x0070E0);
    inline static const Vec4 AccentBright = HexColor(0x26BBFF);
    inline static const Vec4 Button = HexColor(0x2E2E2E);
    inline static const Vec4 ButtonHover = HexColor(0x3A3A3A);
    inline static const Vec4 ButtonPressed = HexColor(0x232323);
    inline static const Vec4 FieldBorder = HexColor(0x3A3A3A);
    inline static const Vec4 CaptionHover = Vec4(1.0f, 1.0f, 1.0f, 0.07f);
    inline static const Vec4 CaptionCloseHover = HexColor(0xC42B1C);
    inline static const Vec4 DropPreview = HexColor(0x26BBFF, 0.28f);

    inline static const Vec4 TransformX = HexColor(0xD84A4A);
    inline static const Vec4 TransformY = HexColor(0x71B33C);
    inline static const Vec4 TransformZ = HexColor(0x3D7BD8);

    // Metrics
    inline static constexpr float TitleBarHeight = 36.0f;
    inline static constexpr float ToolBarHeight = 40.0f;
    inline static constexpr float BottomBarHeight = 34.0f;
    inline static constexpr float TabHeaderHeight = 26.0f;
    inline static constexpr float CaptionButtonWidth = 46.0f;
    inline static constexpr float SplitterThickness = 4.0f;
    inline static constexpr float MinPanelSize = 90.0f;
    inline static constexpr float FontSize = 13.0f;
    inline static constexpr float MacTrafficLightInset = 76.0f;

    static void ApplyButtonStyle(UIButton& InButton, float InFontSize = FontSize) {
        InButton.NormalColor = Button;
        InButton.HoverColor = ButtonHover;
        InButton.PressedColor = ButtonPressed;
        if (Node* child = InButton.GetChildByClass(UILabel::StaticClass())) {
            UILabel* label = child->As<UILabel>();
            label->FontSize = InFontSize;
            label->Color = Text;
        }
    }
};

#include "EditorStyle.h"
#include "Settings/EditorSettings.h"

struct EditorPalette {
    uint32_t TitleBar, ToolBar, BottomBar, Border;
    uint32_t Panel, PanelDark, TabBar, TabActive, TabHover;
    uint32_t Text, TextDim, TextBright;
    uint32_t Accent, AccentBright, Button, ButtonHover, ButtonPressed, FieldBorder;
};

static constexpr EditorPalette s_DarkPalette = {
    0x151515, 0x1D1D1D, 0x181818, 0x0C0C0C,
    0x242424, 0x161616, 0x181818, 0x242424, 0x2D2D2D,
    0xC8C8C8, 0x8F8F8F, 0xFFFFFF,
    0x0070E0, 0x26BBFF, 0x2E2E2E, 0x3A3A3A, 0x232323, 0x3A3A3A,
};

static constexpr EditorPalette s_GraphitePalette = {
    0x202020, 0x2A2A2A, 0x232323, 0x161616,
    0x323232, 0x222222, 0x232323, 0x323232, 0x3C3C3C,
    0xD6D6D6, 0x9E9E9E, 0xFFFFFF,
    0x0070E0, 0x26BBFF, 0x3D3D3D, 0x4A4A4A, 0x2E2E2E, 0x4A4A4A,
};

static constexpr EditorPalette s_ContrastPalette = {
    0x000000, 0x0A0A0A, 0x050505, 0x2A2A2A,
    0x121212, 0x000000, 0x050505, 0x121212, 0x1F1F1F,
    0xEDEDED, 0xB4B4B4, 0xFFFFFF,
    0x1A8CFF, 0x4CC8FF, 0x1C1C1C, 0x2C2C2C, 0x0F0F0F, 0x5A5A5A,
};

static const EditorPalette& GetPalette(EditorTheme InTheme) {
    switch (InTheme) {
        case EditorTheme::Graphite: return s_GraphitePalette;
        case EditorTheme::Contrast: return s_ContrastPalette;
        default: return s_DarkPalette;
    }
}

void EditorStyle::Apply(const EditorAppearanceSettings& InAppearance) {
    const EditorPalette& palette = GetPalette(InAppearance.Theme);
    TitleBar = HexColor(palette.TitleBar);
    ToolBar = HexColor(palette.ToolBar);
    BottomBar = HexColor(palette.BottomBar);
    Border = HexColor(palette.Border);
    Panel = HexColor(palette.Panel);
    PanelDark = HexColor(palette.PanelDark);
    TabBar = HexColor(palette.TabBar);
    TabActive = HexColor(palette.TabActive);
    TabHover = HexColor(palette.TabHover);
    Text = HexColor(palette.Text);
    TextDim = HexColor(palette.TextDim);
    TextBright = HexColor(palette.TextBright);
    Accent = HexColor(palette.Accent);
    AccentBright = HexColor(palette.AccentBright);
    Button = HexColor(palette.Button);
    ButtonHover = HexColor(palette.ButtonHover);
    ButtonPressed = HexColor(palette.ButtonPressed);
    FieldBorder = HexColor(palette.FieldBorder);
    FontSize = InAppearance.FontSize;
}

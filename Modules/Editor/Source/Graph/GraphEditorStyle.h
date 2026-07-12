#pragma once
#include "UI/EditorStyle.h"
#include "Common/Map.h"

/** Colors and metrics of the graph editor. Metrics are in graph units — the view multiplies by
 *  its zoom when painting. */
class GraphEditorStyle {
public:
    // Canvas
    inline static const Vec4 Background = HexColor(0x161616);
    inline static const Vec4 GridMinor = HexColor(0x1D1D1D);
    inline static const Vec4 GridMajor = HexColor(0x101010);
    inline static const Vec4 MarqueeFill = HexColor(0x26BBFF, 0.10f);
    inline static const Vec4 MarqueeBorder = HexColor(0x26BBFF, 0.75f);

    // Nodes
    inline static const Vec4 NodeBody = HexColor(0x1E1E1E, 0.94f);
    inline static const Vec4 NodeShadow = HexColor(0x000000, 0.35f);
    inline static const Vec4 NodeBorder = HexColor(0x0C0C0C);
    inline static const Vec4 NodeBorderSelected = EditorStyle::AccentBright;
    inline static const Vec4 SelectionGlow = HexColor(0x26BBFF, 0.22f);
    inline static const Vec4 HeaderSeparator = HexColor(0x000000, 0.45f);
    inline static const Vec4 TitleColor = EditorStyle::TextBright;
    inline static const Vec4 TitleShadow = HexColor(0x000000, 0.55f);
    inline static const Vec4 PinLabelColor = EditorStyle::Text;

    // Wires
    inline static const Vec4 WireUnderlay = HexColor(0x000000, 0.32f);
    inline static const Vec4 InvalidTargetWire = HexColor(0xE0483C);

    // Context menu
    inline static const Vec4 MenuBackground = HexColor(0x1D1D1D);
    inline static const Vec4 MenuBorder = HexColor(0x0C0C0C);
    inline static const Vec4 MenuHover = HexColor(0x0070E0, 0.55f);

    // Metrics (graph units)
    inline static constexpr float NodeCornerRadius = 7.0f;
    inline static constexpr float HeaderHeight = 26.0f;
    inline static constexpr float RowHeight = 22.0f;
    inline static constexpr float NodePaddingX = 12.0f;
    inline static constexpr float NodeBottomPadding = 8.0f;
    inline static constexpr float NodeMinWidth = 130.0f;
    inline static constexpr float PinColumnGap = 28.0f;
    inline static constexpr float PinRadius = 5.0f;
    inline static constexpr float PinLabelGap = 9.0f;
    inline static constexpr float PinHitRadius = 12.0f;
    inline static constexpr float TitleFontSize = 13.0f;
    inline static constexpr float PinFontSize = 12.0f;
    inline static constexpr float WireThickness = 2.25f;

    inline static constexpr float GridSize = 16.0f;
    inline static constexpr int GridMajorEvery = 8;
    inline static constexpr float SnapSize = 16.0f;
    inline static constexpr float ZoomMin = 0.25f;
    inline static constexpr float ZoomMax = 2.5f;
    /** Zoom below which node text is dropped as unreadable. */
    inline static constexpr float TextCullZoom = 0.35f;

    // Context menu metrics (screen pixels; the menu does not zoom)
    inline static constexpr float MenuWidth = 220.0f;
    inline static constexpr float MenuRowHeight = 24.0f;
    inline static constexpr float MenuHeaderHeight = 20.0f;
    inline static constexpr float MenuPadding = 4.0f;

    static Vec4 PinColor(const String& InTypeName) {
        static Map<String, Vec4> s_Colors = BuildPinColors();
        return s_Colors.GetOrDefault(InTypeName, HexColor(0xB8B8B8));
    }

private:
    static Map<String, Vec4> BuildPinColors() {
        Map<String, Vec4> colors;
        colors["Exec"] = HexColor(0xE8E8E8);
        colors["Bool"] = HexColor(0xA02B2B);
        colors["Int"] = HexColor(0x2FD5A8);
        colors["Float"] = HexColor(0x9BE64A);
        colors["Vec2"] = HexColor(0xF2C94C);
        colors["Vec3"] = HexColor(0xF2C94C);
        colors["Vec4"] = HexColor(0xF2C94C);
        colors["Color"] = HexColor(0x3F8FFF);
        colors["String"] = HexColor(0xD75DD7);
        return colors;
    }
};

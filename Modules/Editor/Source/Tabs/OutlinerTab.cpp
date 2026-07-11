#include "OutlinerTab.h"
#include "UI/EditorStyle.h"
#include "GameFramework/UIBuilder.h"
#include "GameFramework/UIScrollArea.h"

struct OutlinerRow {
    const char* Name;
    const char* NodeClass;
};

static const OutlinerRow s_PlaceholderRows[] = {
    { "MyLevel", "World" },
    { "SkyLightNode", "SkyLightNode" },
    { "Cube", "MeshNode" },
    { "DirectionalLightNode", "DirectionalLightNode" },
    { "CameraNode", "CameraNode" },
    { "Cube [1]", "MeshNode" },
    { "SSAO", "SSAO" },
    { "PointLightNode", "PointLightNode" },
    { "BoxShapeNode", "BoxShapeNode" },
};

OutlinerTab::OutlinerTab() {
    UIScrollArea* scroll = Add<UIScrollArea>();
    scroll->Fill();
    scroll->Padding = UIPadding(6.0f, 4.0f);

    UI::ForEach(*scroll, [] { return (int)std::size(s_PlaceholderRows); }, [](UINode& InItem, int InIndex) {
        InItem.Size = { 1.0_rel, 22.0_px };
        const OutlinerRow row = s_PlaceholderRows[InIndex];

        UILabel& name = UI::Label(InItem, [row] { return String(row.Name); });
        name.FontSize = EditorStyle::FontSize;
        name.Color = EditorStyle::Text;
        name.VAlign = UIVAlign::Middle;

        UILabel& nodeClass = UI::Label(InItem, [row] { return String(row.NodeClass); });
        nodeClass.FontSize = EditorStyle::FontSize;
        nodeClass.Color = EditorStyle::TextDim;
        nodeClass.HAlign = UIHAlign::Right;
        nodeClass.VAlign = UIVAlign::Middle;
    });
}

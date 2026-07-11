#include "DetailsTab.h"
#include "UI/EditorStyle.h"
#include "GameFramework/UIBuilder.h"
#include "GameFramework/UIImage.h"
#include <string>

static int s_Count = 0;

DetailsTab::DetailsTab() {
    UI::VStack(*this, [](UIStack& InStack) {
        InStack.Padding = UIPadding(8.0f);
        InStack.Gap = 6.0f;

        UILabel& count = UI::Label(InStack, [] { return "Count: " + std::to_string(s_Count); });
        count.Size = { 1.0_rel, 18.0_px };
        count.FontSize = EditorStyle::FontSize;
        count.Color = EditorStyle::Text;

        UIButton& add = UI::Button(InStack, "Add", [] { s_Count++; });
        add.Size = { 1.0_rel, 24.0_px };
        EditorStyle::ApplyButtonStyle(add);

        UIImage* progress = InStack.Add<UIImage>();
        progress->Size = Vec2(24.0f, 24.0f);
        progress->FillMethod = UIImageFill::Radial;
        progress->Tint = EditorStyle::Accent;
        progress->Bind = [progress] { progress->FillAmount = (float)(s_Count % 10) / 10.0f; };
    });
}

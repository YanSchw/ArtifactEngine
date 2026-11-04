#include "EditorSettingsDetailsCustomization.h"
#include "DetailsCategory.h"
#include "DetailsRow.h"
#include "Tabs/DetailsTab.h"
#include "UI/EditorStyle.h"
#include "UI/UIDropdown.h"
#include "Assets/AssetManager.h"
#include "Assets/Font.h"
#include "GameFramework/UILabel.h"

static const Vec4 s_NoticeColor = HexColor(0xE0A63A);

static Array<Asset*> GetFontAssets() {
    Array<Asset*> fonts = AssetManager::Get().GetAssetsOfClass(Font::StaticClass());
    fonts.Sort([](Asset* const& InA, Asset* const& InB) {
        return InA->GetDisplayName() < InB->GetDisplayName();
    });
    return fonts;
}

static UUID* ResolveFontId(const WeakObjectPtr<Object>& InObject) {
    EditorSettings* settings = Cast<EditorSettings>(InObject.Get());
    return settings ? &settings->Appearance.InterfaceFont : nullptr;
}

float EditorSettingsDetailsCustomization::BuildHeader(UINode& InHeader, Object* InObject, DetailsTab& InTab) {
    const float height = DetailsCustomization::BuildHeader(InHeader, InObject, InTab);

    UILabel* notice = InHeader.Add<UILabel>();
    notice->Fill();
    notice->Padding = UIPadding(0.0f, 0.0f, 12.0f, 0.0f);
    notice->FontSize = EditorStyle::FontSize;
    notice->Color = s_NoticeColor;
    notice->HAlign = UIHAlign::Right;
    notice->VAlign = UIVAlign::Middle;
    notice->Bind = [notice] {
        notice->Text = EditorSettings::Get().NeedsRestart() ? "Restart the editor to apply the theme and font size" : "";
    };
    return height;
}

void EditorSettingsDetailsCustomization::BuildContent(UINode& InList, Object* InObject, DetailsTab& InTab) {
    const WeakObjectPtr<Object> weak(InObject);
    for (Property* group : Property::GetTypeProperties(EditorSettings::StaticClass().Name)) {
        DetailsCategory& category = AddCategory(InList, InTab, PrettyPropertyName(group->Name), 0);
        for (Property* property : GetInnerProperties(group)) {
            if (property->Name == "InterfaceFont") {
                AddFontRow(*category.GetBody(), InTab, weak, property);
            } else {
                AddPropertyRow(*category.GetBody(), InTab, weak, group->Offset, property, 0);
            }
        }
    }
}

void EditorSettingsDetailsCustomization::AddFontRow(UINode& InParent, DetailsTab& InTab, const WeakObjectPtr<Object>& InObject,
                                                    Property* InProperty) {
    DetailsRow& row = AddRow(InParent, InTab, PrettyPropertyName(InProperty->Name), 0);
    UIDropdown* dropdown = row.GetValueHost()->Add<UIDropdown>();
    dropdown->Fill();
    dropdown->GetOptions = [] {
        Array<UIDropdownOption> options;
        for (Asset* font : GetFontAssets()) {
            options.Add(font->GetDisplayName());
        }
        return options;
    };
    dropdown->GetSelectedIndex = [InObject]() -> int32_t {
        const UUID* id = ResolveFontId(InObject);
        const Array<Asset*> fonts = GetFontAssets();
        for (int32_t i = 0; id && i < fonts.Size(); i++) {
            if (fonts[i]->GetId() == *id) {
                return i;
            }
        }
        return -1;
    };
    dropdown->GetSelectedLabel = [InObject] {
        const UUID* id = ResolveFontId(InObject);
        Asset* font = id ? AssetManager::Get().GetAsset(*id) : nullptr;
        return font ? font->GetDisplayName() : String("None");
    };
    const WeakObjectPtr<DetailsTab> tab(&InTab);
    dropdown->SelectionChanged = [InObject, InProperty, tab](int32_t InIndex) {
        const Array<Asset*> fonts = GetFontAssets();
        UUID* id = ResolveFontId(InObject);
        if (!id || !tab.Get() || InIndex < 0 || InIndex >= fonts.Size()) {
            return;
        }
        tab.Get()->RecordEdit("Edit " + PrettyPropertyName(InProperty->Name), InObject.Get());
        *id = fonts[InIndex]->GetId();
        NotifyPropertyEdited(tab.Get(), InObject.Get(), InProperty->Name);
    };
}

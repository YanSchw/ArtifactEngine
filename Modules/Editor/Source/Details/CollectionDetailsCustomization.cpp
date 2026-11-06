#include "CollectionDetailsCustomization.h"

#include "DetailsCategory.h"
#include "DetailsRow.h"
#include "UI/UIDropdown.h"
#include "Rendering/ShaderTemplate.h"
#include <memory>

void CollectionDetailsCustomization::BuildClassCategory(DetailsCategory& InCategory, const Class& InClass, Object* InObject, DetailsTab& InTab) {
    if (InClass != CollectionAsset::StaticClass()) {
        AssetDetailsCustomization::BuildClassCategory(InCategory, InClass, InObject, InTab);
        return;
    }

    const WeakObjectPtr<Object> weak(InObject);
    for (Property* property : Property::GetTypeProperties(InClass.Name)) {
        if (property->Name != "m_ImportSettings") {
            AddPropertyRow(*InCategory.GetBody(), InTab, weak, 0, property, 0);
            continue;
        }
        for (Property* group : GetInnerProperties(property)) {
            DetailsCategory& category = AddCategory(*InCategory.GetBody(), InTab, PrettyPropertyName(group->Name), 1);
            const uint64_t groupOffset = property->Offset + group->Offset;
            for (Property* setting : GetInnerProperties(group)) {
                if (setting->Name == "ShaderTemplate") {
                    AddTemplateRow(*category.GetBody(), InTab, weak, groupOffset + setting->Offset, property);
                } else {
                    AddPropertyRow(*category.GetBody(), InTab, weak, groupOffset, setting, 1, property);
                }
            }
        }
    }
}

void CollectionDetailsCustomization::AddTemplateRow(UINode& InBody, DetailsTab& InTab, const WeakObjectPtr<Object>& InObject,
                                                    uint64_t InOffset, Property* InRootProperty) {
    DetailsRow& row = AddRow(InBody, InTab, "Shader Template", 1);
    const DetailsEditHandler onEdited = MakeEditHandler(InObject, InRootProperty, &InTab);
    const auto templates = std::make_shared<Array<ShaderTemplateInfo>>(ShaderTemplate::FindAll());
    const auto read = [InObject, InOffset]() -> String* {
        Object* object = InObject.Get();
        return object ? (String*)((char*)object + InOffset) : nullptr;
    };
    const auto indexOf = [templates](const String& InPath) {
        for (int32_t i = 0; i < templates->Size(); i++) {
            if ((*templates)[i].Path == InPath) {
                return i;
            }
        }
        return -1;
    };

    UIDropdown* dropdown = row.GetValueHost()->Add<UIDropdown>();
    dropdown->Fill();
    dropdown->GetOptions = [templates] {
        Array<UIDropdownOption> options;
        for (const ShaderTemplateInfo& info : *templates) {
            options.Add(UIDropdownOption(info.DisplayName));
        }
        return options;
    };
    dropdown->GetSelectedIndex = [read, indexOf] {
        String* path = read();
        return path ? indexOf(*path) : -1;
    };
    dropdown->GetSelectedLabel = [read, indexOf, templates] {
        String* path = read();
        const int32_t index = path ? indexOf(*path) : -1;
        return index >= 0 ? (*templates)[index].DisplayName : (path ? *path : String());
    };
    dropdown->SelectionChanged = [read, templates, onEdited](int32_t InIndex) {
        String* path = read();
        if (path && InIndex >= 0 && InIndex < templates->Size()) {
            onEdited.Record();
            *path = (*templates)[InIndex].Path;
            onEdited();
        }
    };
}

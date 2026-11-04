#include "InputMappingDetailsCustomization.h"
#include "DetailsCategory.h"
#include "DetailsRow.h"
#include "Tabs/DetailsTab.h"
#include "Tabs/InputMappingEditorTab.h"
#include "UI/EditorStyle.h"
#include "UI/UIDropdown.h"
#include "GameFramework/UILabel.h"

static const String s_NoControl = "None";

DetailsEditHandler InputMappingDetailsCustomization::MakeMappingEditHandler(DetailsTab& InTab, Object* InObject, Property* InProperty) {
    const WeakObjectPtr<DetailsTab> tab = &InTab;
    const WeakObjectPtr<Object> target = InObject;

    DetailsEditHandler handler;
    handler.BeginEdit = [tab, InProperty] {
        MajorTab* major = tab.Get() ? tab.Get()->GetMajorTab() : nullptr;
        if (major && major->GetEditedAsset()) {
            major->BeginTransaction("Edit " + PrettyPropertyName(InProperty->Name), major->GetEditedAsset());
        }
    };
    handler.CommitEdit = [tab, target, InProperty] {
        if (Object* object = target.Get()) {
            InProperty->NotifyChanged(object);
            NotifyPropertyEdited(tab.Get(), object, InProperty->Name);
        }
    };
    return handler;
}

void InputMappingDetailsCustomization::AddValueRow(UINode& InParent, DetailsTab& InTab, Object* InObject, Property* InProperty) {
    DetailsRow& row = AddRow(InParent, InTab, PrettyPropertyName(InProperty->Name), 0);
    BuildValueEditor(*row.GetValueHost(), InObject, InProperty->Offset, InProperty,
                     MakeMappingEditHandler(InTab, InObject, InProperty), row.GetLabel());
}

void InputActionDetailsCustomization::BuildContent(UINode& InList, Object* InObject, DetailsTab& InTab) {
    DetailsCategory& category = AddCategory(InList, InTab, "Action", 0);
    for (Property* property : Property::GetAllTypeProperties(InObject->GetClass())) {
        if (!Cast<ArrayProperty>(property)) {
            AddValueRow(*category.GetBody(), InTab, InObject, property);
        }
    }
}

float InputBindingDetailsCustomization::BuildHeader(UINode& InHeader, Object* InObject, DetailsTab& InTab) {
    (void)InTab;
    UILabel* label = InHeader.Add<UILabel>();
    label->Fill();
    label->Position = Vec2(10.0f, 0.0f);
    label->FontSize = EditorStyle::FontSize + 1.0f;
    label->Color = EditorStyle::TextBright;
    label->VAlign = UIVAlign::Middle;
    label->Text = InputMappingEditorTab::GetBindingLabel(InObject->GetClass()) + " Binding";
    return 32.0f;
}

void InputBindingDetailsCustomization::BuildContent(UINode& InList, Object* InObject, DetailsTab& InTab) {
    DetailsCategory& category = AddCategory(InList, InTab, "Binding", 0);
    for (Property* property : Property::GetAllTypeProperties(InObject->GetClass())) {
        if (Cast<StringProperty>(property)) {
            AddControlPathRow(*category.GetBody(), InTab, InObject, property);
        } else {
            AddValueRow(*category.GetBody(), InTab, InObject, property);
        }
    }
}

void InputBindingDetailsCustomization::AddControlPathRow(UINode& InParent, DetailsTab& InTab, Object* InObject, Property* InProperty) {
    DetailsRow& row = AddRow(InParent, InTab, PrettyPropertyName(InProperty->Name), 0);

    const WeakObjectPtr<Object> binding = InObject;
    const WeakObjectPtr<DetailsTab> tab = &InTab;
    const uint64_t offset = InProperty->Offset;
    const auto path = [binding, offset]() -> String* {
        Object* object = binding.Get();
        return object ? (String*)((char*)object + offset) : nullptr;
    };
    const auto controlPaths = [binding, tab]() -> Array<String> {
        MajorTab* major = tab.Get() ? tab.Get()->GetMajorTab() : nullptr;
        InputMappingEditorTab* editor = major ? major->As<InputMappingEditorTab>() : nullptr;
        return editor ? editor->GetControlPathsFor(Cast<InputBinding>(binding.Get())) : Array<String>();
    };

    UIDropdown* dropdown = row.GetValueHost()->Add<UIDropdown>();
    dropdown->Fill();
    dropdown->SearchPlaceholder = "Search controls";
    dropdown->GetSelectedLabel = [path] {
        const String* current = path();
        return current && !current->empty() ? *current : s_NoControl;
    };
    dropdown->GetOptions = [controlPaths] {
        Array<UIDropdownOption> options = { UIDropdownOption(s_NoControl) };
        for (const String& controlPath : controlPaths()) {
            options.Add(controlPath);
        }
        return options;
    };
    dropdown->GetSelectedIndex = [path, controlPaths]() -> int32_t {
        const String* current = path();
        if (!current || current->empty()) {
            return 0;
        }
        const Array<String> paths = controlPaths();
        for (int32_t i = 0; i < paths.Size(); i++) {
            if (paths[i] == *current) {
                return i + 1;
            }
        }
        return -1;
    };

    const DetailsEditHandler onEdited = MakeMappingEditHandler(InTab, InObject, InProperty);
    dropdown->SelectionChanged = [path, controlPaths, onEdited](int32_t InIndex) {
        const Array<String> paths = controlPaths();
        String* current = path();
        if (!current || InIndex < 0 || InIndex > paths.Size()) {
            return;
        }
        onEdited.Record();
        *current = InIndex == 0 ? String() : paths[InIndex - 1];
        onEdited();
    };
}

#include "DetailsCustomization.h"
#include "DetailsCategory.h"
#include "DetailsRow.h"
#include "Tabs/DetailsTab.h"
#include "UI/EditorStyle.h"
#include "UI/UIDragNumber.h"
#include "UI/UIDropdown.h"
#include "UI/UICheckbox.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UITextArea.h"
#include "Object/Enum.h"
#include "Assets/AssetManager.h"
#include "Assets/Asset.h"
#include "Common/Map.h"
#include <cctype>
#include <cstring>
#include <memory>

static char* ResolveBase(const WeakObjectPtr<Object>& InObject, uint64_t InOffset) {
    Object* object = InObject.Get();
    return object ? (char*)object + InOffset : nullptr;
}

static double ReadIntValue(const IntProperty& InProperty, const void* InPtr) {
    if (InProperty.IsUnsigned) {
        switch (InProperty.NumBits) {
            case 8:  return (double)*(const uint8_t*)InPtr;
            case 16: return (double)*(const uint16_t*)InPtr;
            case 32: return (double)*(const uint32_t*)InPtr;
            case 64: return (double)*(const uint64_t*)InPtr;
        }
    } else {
        switch (InProperty.NumBits) {
            case 8:  return (double)*(const int8_t*)InPtr;
            case 16: return (double)*(const int16_t*)InPtr;
            case 32: return (double)*(const int32_t*)InPtr;
            case 64: return (double)*(const int64_t*)InPtr;
        }
    }
    return 0.0;
}

static void WriteIntValue(const IntProperty& InProperty, void* InPtr, double InValue) {
    if (InProperty.IsUnsigned) {
        const uint64_t value = (uint64_t)InValue;
        switch (InProperty.NumBits) {
            case 8:  *(uint8_t*)InPtr = (uint8_t)value; break;
            case 16: *(uint16_t*)InPtr = (uint16_t)value; break;
            case 32: *(uint32_t*)InPtr = (uint32_t)value; break;
            case 64: *(uint64_t*)InPtr = value; break;
        }
    } else {
        const int64_t value = (int64_t)InValue;
        switch (InProperty.NumBits) {
            case 8:  *(int8_t*)InPtr = (int8_t)value; break;
            case 16: *(int16_t*)InPtr = (int16_t)value; break;
            case 32: *(int32_t*)InPtr = (int32_t)value; break;
            case 64: *(int64_t*)InPtr = value; break;
        }
    }
}

static Object* ReadObjectPtr(void* InPtr, bool InIsWeak) {
    return InIsWeak ? ((WeakObjectPtr<Object>*)InPtr)->Get() : ((SharedObjectPtr<Object>*)InPtr)->Get();
}

static void WriteObjectPtr(void* InPtr, bool InIsWeak, Object* InValue) {
    if (InIsWeak) {
        *(WeakObjectPtr<Object>*)InPtr = InValue;
    } else {
        *(SharedObjectPtr<Object>*)InPtr = InValue;
    }
}

static UILabel& AddValueLabel(UINode& InHost, const String& InText) {
    UILabel* label = InHost.Add<UILabel>();
    label->Fill();
    label->Position = Vec2(6.0f, 0.0f);
    label->FontSize = EditorStyle::FontSize;
    label->Color = EditorStyle::TextDim;
    label->VAlign = UIVAlign::Middle;
    label->Text = InText;
    return *label;
}

static void BuildNumberRow(DetailsRow& InRow, const WeakObjectPtr<Object>& InObject, uint64_t InOffset, Property* InProperty) {
    UIDragNumber* drag = InRow.GetValueHost()->Add<UIDragNumber>();
    drag->Fill();
    if (IntProperty* intProperty = Cast<IntProperty>(InProperty)) {
        drag->Integer = true;
        drag->Sensitivity = 0.2;
        if (intProperty->IsUnsigned) {
            drag->MinValue = 0.0;
        }
        drag->Get = [InObject, InOffset, intProperty]() -> double {
            char* base = ResolveBase(InObject, InOffset);
            return base ? ReadIntValue(*intProperty, base) : 0.0;
        };
        drag->Set = [InObject, InOffset, intProperty](double InValue) {
            if (char* base = ResolveBase(InObject, InOffset)) {
                WriteIntValue(*intProperty, base, InValue);
            }
        };
    } else if (FloatProperty* floatProperty = Cast<FloatProperty>(InProperty)) {
        drag->Get = [InObject, InOffset, floatProperty]() -> double {
            char* base = ResolveBase(InObject, InOffset);
            if (!base) {
                return 0.0;
            }
            return floatProperty->IsDouble ? *(double*)base : (double)*(float*)base;
        };
        drag->Set = [InObject, InOffset, floatProperty](double InValue) {
            if (char* base = ResolveBase(InObject, InOffset)) {
                if (floatProperty->IsDouble) {
                    *(double*)base = InValue;
                } else {
                    *(float*)base = (float)InValue;
                }
            }
        };
    }
}

static void BuildBoolRow(DetailsRow& InRow, const WeakObjectPtr<Object>& InObject, uint64_t InOffset) {
    UICheckbox* checkbox = InRow.GetValueHost()->Add<UICheckbox>();
    checkbox->Anchor = checkbox->Pivot = Vec2(0.0f, 0.5f);
    checkbox->Position = Vec2(2.0f, 0.0f);
    checkbox->Bind = [checkbox, InObject, InOffset] {
        if (char* base = ResolveBase(InObject, InOffset)) {
            checkbox->IsOn = *(bool*)base;
        }
    };
    checkbox->Changed = [InObject, InOffset](bool InValue) {
        if (char* base = ResolveBase(InObject, InOffset)) {
            *(bool*)base = InValue;
        }
    };
}

static void BuildStringRow(DetailsRow& InRow, const WeakObjectPtr<Object>& InObject, uint64_t InOffset) {
    UITextArea* field = InRow.GetValueHost()->Add<UITextArea>();
    field->Fill();
    field->SingleLine = true;
    field->FontSize = EditorStyle::FontSize;
    field->TextColor = EditorStyle::Text;
    field->CaretColor = EditorStyle::TextBright;
    field->BackgroundColor = EditorStyle::PanelDark;
    field->FocusedBorderColor = EditorStyle::Accent;
    field->CornerRadius = 2.0f;
    field->Padding = UIPadding(6.0f, 0.0f);
    field->Bind = [field, InObject, InOffset] {
        if (!field->IsFocused()) {
            if (char* base = ResolveBase(InObject, InOffset)) {
                field->Text = *(String*)base;
            }
        }
    };
    const auto commit = [InObject, InOffset](const String& InText) {
        if (char* base = ResolveBase(InObject, InOffset)) {
            *(String*)base = InText;
        }
    };
    field->Submitted = commit;
    field->FocusLost = [field, commit] { commit(field->Text); };
}

static void BuildEnumRow(DetailsRow& InRow, const WeakObjectPtr<Object>& InObject, uint64_t InOffset, EnumProperty* InProperty) {
    const auto read = [InObject, InOffset, InProperty]() -> int64_t {
        int64_t value = 0;
        if (char* base = ResolveBase(InObject, InOffset)) {
            std::memcpy(&value, base, InProperty->ByteSize);
        }
        return value;
    };

    UIDropdown* dropdown = InRow.GetValueHost()->Add<UIDropdown>();
    dropdown->Fill();
    dropdown->GetSelectedLabel = [read, InProperty] {
        return Enum(InProperty->InnerEnumTypename).ConvertValueToString(read());
    };
    dropdown->GetOptions = [InProperty] {
        Array<String> options;
        for (const Enum::EnumValue& value : Enum(InProperty->InnerEnumTypename).GetValues()) {
            options.Add(value.Name);
        }
        return options;
    };
    dropdown->GetSelectedIndex = [read, InProperty]() -> int32_t {
        const Array<Enum::EnumValue>& values = Enum(InProperty->InnerEnumTypename).GetValues();
        for (int32_t i = 0; i < values.Size(); i++) {
            if (values[i].Value == read()) {
                return i;
            }
        }
        return -1;
    };
    dropdown->SelectionChanged = [InObject, InOffset, InProperty](int32_t InIndex) {
        const Array<Enum::EnumValue>& values = Enum(InProperty->InnerEnumTypename).GetValues();
        if (InIndex < 0 || InIndex >= values.Size()) {
            return;
        }
        if (char* base = ResolveBase(InObject, InOffset)) {
            std::memcpy(base, &values[InIndex].Value, InProperty->ByteSize);
        }
    };
}

static void BuildAssetRow(DetailsRow& InRow, const WeakObjectPtr<Object>& InObject, uint64_t InOffset,
                          const Class& InAssetClass, bool InIsWeak) {
    auto snapshot = std::make_shared<Array<WeakObjectPtr<Asset>>>();

    UIDropdown* dropdown = InRow.GetValueHost()->Add<UIDropdown>();
    dropdown->Fill();
    dropdown->GetSelectedLabel = [InObject, InOffset, InIsWeak] {
        char* base = ResolveBase(InObject, InOffset);
        Asset* asset = base ? Cast<Asset>(ReadObjectPtr(base, InIsWeak)) : nullptr;
        return asset ? asset->GetDisplayName() : String("None");
    };
    dropdown->GetOptions = [snapshot, InAssetClass] {
        snapshot->Clear();
        Array<String> options;
        options.Add("None");
        for (Asset* asset : AssetManager::Get().GetAssetsOfClass(InAssetClass)) {
            snapshot->Add(WeakObjectPtr<Asset>(asset));
            options.Add(asset->GetDisplayName());
        }
        return options;
    };
    dropdown->GetSelectedIndex = [snapshot, InObject, InOffset, InIsWeak]() -> int32_t {
        char* base = ResolveBase(InObject, InOffset);
        Object* current = base ? ReadObjectPtr(base, InIsWeak) : nullptr;
        for (int32_t i = 0; i < snapshot->Size(); i++) {
            if ((*snapshot)[i].Get() == current) {
                return i + 1;
            }
        }
        return 0;
    };
    dropdown->SelectionChanged = [snapshot, InObject, InOffset, InIsWeak](int32_t InIndex) {
        Asset* asset = (InIndex > 0 && InIndex <= snapshot->Size()) ? (*snapshot)[InIndex - 1].Get() : nullptr;
        if (char* base = ResolveBase(InObject, InOffset)) {
            WriteObjectPtr(base, InIsWeak, asset);
        }
    };
}

float DetailsCustomization::BuildHeader(UINode& InHeader, Object* InObject, DetailsTab& InTab) {
    (void)InTab;
    UILabel* label = InHeader.Add<UILabel>();
    label->Fill();
    label->Position = Vec2(10.0f, 0.0f);
    label->FontSize = EditorStyle::FontSize + 1.0f;
    label->Color = EditorStyle::TextBright;
    label->VAlign = UIVAlign::Middle;
    label->Text = PrettyClassName(InObject->GetClass());
    return 32.0f;
}

void DetailsCustomization::BuildContent(UINode& InList, Object* InObject, DetailsTab& InTab) {
    Array<Class> chain;
    for (Class current = InObject->GetClass(); current != Class::None && current.Name != "Object"; current = current.GetParentClass()) {
        chain.Add(current);
    }
    for (int32_t i = chain.Size() - 1; i >= 0; i--) {
        if (!WantsClassCategory(chain[i])) {
            continue;
        }
        DetailsCategory& category = AddCategory(InList, InTab, PrettyClassName(chain[i]), 0);
        BuildClassCategory(category, chain[i], InObject, InTab);
    }
}

bool DetailsCustomization::WantsClassCategory(const Class& InClass) const {
    return !Property::GetTypeProperties(InClass.Name).IsEmpty();
}

void DetailsCustomization::BuildClassCategory(DetailsCategory& InCategory, const Class& InClass, Object* InObject, DetailsTab& InTab) {
    WeakObjectPtr<Object> weak(InObject);
    for (Property* property : Property::GetTypeProperties(InClass.Name)) {
        AddPropertyRow(*InCategory.GetBody(), InTab, weak, 0, property, 0);
    }
}

DetailsCategory& DetailsCustomization::AddCategory(UINode& InParent, DetailsTab& InTab, const String& InTitle, int32_t InDepth) {
    DetailsCategory* category = InParent.Add<DetailsCategory>();
    category->OwnerTab = &InTab;
    category->Depth = InDepth;
    category->SetTitle(InTitle);
    return *category;
}

DetailsRow& DetailsCustomization::AddRow(UINode& InParent, DetailsTab& InTab, const String& InLabel, int32_t InDepth) {
    DetailsRow* row = InParent.Add<DetailsRow>();
    row->OwnerTab = &InTab;
    row->Depth = InDepth;
    row->SetLabel(InLabel);
    return *row;
}

void DetailsCustomization::AddPropertyRow(UINode& InParent, DetailsTab& InTab, const WeakObjectPtr<Object>& InObject,
                                          uint64_t InBaseOffset, Property* InProperty, int32_t InDepth) {
    const uint64_t offset = InBaseOffset + InProperty->Offset;

    if (StructProperty* structProperty = Cast<StructProperty>(InProperty)) {
        const Array<Property*> inner = Property::GetTypeProperties(structProperty->InnerStructTypename);
        if (!inner.IsEmpty()) {
            DetailsCategory& category = AddCategory(InParent, InTab, InProperty->Name, InDepth + 1);
            for (Property* property : inner) {
                AddPropertyRow(*category.GetBody(), InTab, InObject, offset, property, InDepth + 1);
            }
            return;
        }
        AddValueLabel(*AddRow(InParent, InTab, InProperty->Name, InDepth).GetValueHost(), structProperty->InnerStructTypename);
        return;
    }

    DetailsRow& row = AddRow(InParent, InTab, InProperty->Name, InDepth);

    if (Cast<IntProperty>(InProperty) || Cast<FloatProperty>(InProperty)) {
        BuildNumberRow(row, InObject, offset, InProperty);
    } else if (Cast<BoolProperty>(InProperty)) {
        BuildBoolRow(row, InObject, offset);
    } else if (Cast<StringProperty>(InProperty)) {
        BuildStringRow(row, InObject, offset);
    } else if (EnumProperty* enumProperty = Cast<EnumProperty>(InProperty)) {
        BuildEnumRow(row, InObject, offset, enumProperty);
    } else if (SharedObjectPtrProperty* sharedProperty = Cast<SharedObjectPtrProperty>(InProperty)) {
        if (sharedProperty->InnerClass.IsSubclassOf(Asset::StaticClass())) {
            BuildAssetRow(row, InObject, offset, sharedProperty->InnerClass, false);
        } else {
            AddValueLabel(*row.GetValueHost(), sharedProperty->InnerClass.Name);
        }
    } else if (WeakObjectPtrProperty* weakProperty = Cast<WeakObjectPtrProperty>(InProperty)) {
        if (weakProperty->InnerClass.IsSubclassOf(Asset::StaticClass())) {
            BuildAssetRow(row, InObject, offset, weakProperty->InnerClass, true);
        } else {
            AddValueLabel(*row.GetValueHost(), weakProperty->InnerClass.Name);
        }
    } else if (ArrayProperty* arrayProperty = Cast<ArrayProperty>(InProperty)) {
        UILabel& label = AddValueLabel(*row.GetValueHost(), "");
        label.Bind = [label = &label, InObject, offset, arrayProperty] {
            char* base = ResolveBase(InObject, offset);
            label->Text = base ? std::to_string(arrayProperty->GetSize(base)) + " elements" : "";
        };
    } else {
        AddValueLabel(*row.GetValueHost(), "Unsupported");
    }
}

String DetailsCustomization::PrettyClassName(const Class& InClass) {
    String pretty;
    for (size_t i = 0; i < InClass.Name.size(); i++) {
        const char current = InClass.Name[i];
        if (i > 0 && std::isupper((unsigned char)current) && std::islower((unsigned char)InClass.Name[i - 1])) {
            pretty += ' ';
        }
        pretty += current;
    }
    return pretty;
}

DetailsCustomization* DetailsCustomization::FindFor(const Class& InClass) {
    static Map<String, SharedObjectPtr<DetailsCustomization>> s_Instances;

    const auto inheritanceDepth = [](Class InCurrent) {
        int32_t depth = 0;
        while (InCurrent != Class::None && InCurrent.Name != "Object") {
            InCurrent = InCurrent.GetParentClass();
            depth++;
        }
        return depth;
    };

    DetailsCustomization* best = nullptr;
    int32_t bestDepth = -1;
    for (const Class& candidate : Class::GetSubclassesOf(StaticClass())) {
        if (candidate == StaticClass()) {
            continue;
        }
        if (!s_Instances.ContainsKey(candidate.Name)) {
            s_Instances[candidate.Name] = SharedObjectPtr<DetailsCustomization>(Object::Create<DetailsCustomization>(candidate));
        }
        DetailsCustomization* customization = s_Instances[candidate.Name].Get();
        if (!customization) {
            continue;
        }
        const Class supported = customization->GetSupportedClass();
        if (supported == Class::None || !InClass.IsSubclassOf(supported)) {
            continue;
        }
        const int32_t depth = inheritanceDepth(supported);
        if (depth > bestDepth) {
            best = customization;
            bestDepth = depth;
        }
    }
    return best;
}

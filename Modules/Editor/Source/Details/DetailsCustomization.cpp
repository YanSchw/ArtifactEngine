#include "DetailsCustomization.h"
#include "DetailsCategory.h"
#include "DetailsRow.h"
#include "Tabs/DetailsTab.h"
#include "Tabs/MajorTab.h"
#include "UI/EditorStyle.h"
#include "UI/EditorIcons.h"
#include "UI/UIDragNumber.h"
#include "UI/UIDropdown.h"
#include "UI/UIAssetSlot.h"
#include "UI/UICheckbox.h"
#include "UI/UIColorSwatch.h"
#include "UI/UICurveEditor.h"
#include "UI/UIGradientEditor.h"
#include "Assets/Blueprint.h"
#include "Core/Log.h"
#include "HeroTools/ThumbnailRenderer.h"
#include "GameFramework/UIHStack.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UIQuad.h"
#include "GameFramework/UISvg.h"
#include "GameFramework/UIImage.h"
#include "GameFramework/UITextArea.h"
#include "Rendering/Texture.h"
#include "Object/Enum.h"
#include "Assets/AssetManager.h"
#include "Assets/Asset.h"
#include "GameFramework/Node.h"
#include "Common/Map.h"
#include "Particles/ParticleModules.h"
#include "Serialization/Json.h"
#include "Serialization/ThirdParty/nlohmann/json.hpp"
#include "UI/UIContextMenu.h"
#include "UI/UIMenuModel.h"
#include "UI/UITooltip.h"
#include "GameFramework/UIButton.h"
#include <cctype>
#include <cstring>
#include <memory>

static constexpr float AssetRowHeight = 52.0f;

struct ContainerElementAddress {
    uint64_t ContainerOffset = 0;
    ArrayProperty* Array = nullptr;
    MapProperty* Map = nullptr;
    int32_t Index = 0;
    String KeyText;
    std::shared_ptr<void> Key;
    std::shared_ptr<void> PendingKey;
    String ExternalKey;
    std::function<void*(Object*)> Resolve;
};

static constexpr uint64_t s_ElementBit = 1ull << 63;
static constexpr uint64_t s_MapKeyBit = 1ull << 62;
static constexpr uint64_t s_ElementMemberMask = 0xFFFFFFFFull;

static Array<ContainerElementAddress>& GetElementAddresses() {
    static Array<ContainerElementAddress> s_Addresses;
    return s_Addresses;
}

static int32_t ElementSlot(uint64_t InOffset) {
    return (int32_t)((InOffset & ~(s_ElementBit | s_MapKeyBit)) >> 32);
}

static uint64_t ElementOffset(int32_t InSlot) {
    return s_ElementBit | ((uint64_t)InSlot << 32);
}

static uint64_t ArrayElementOffset(uint64_t InArrayOffset, ArrayProperty* InArray, int32_t InIndex) {
    Array<ContainerElementAddress>& addresses = GetElementAddresses();
    for (int32_t i = 0; i < addresses.Size(); i++) {
        const ContainerElementAddress& address = addresses[i];
        if (address.ContainerOffset == InArrayOffset && address.Array == InArray && address.Index == InIndex) {
            return ElementOffset(i);
        }
    }
    ContainerElementAddress address;
    address.ContainerOffset = InArrayOffset;
    address.Array = InArray;
    address.Index = InIndex;
    addresses.Add(address);
    return ElementOffset(addresses.Last());
}

static nlohmann::json ReadMapKey(const MapProperty& InMap, const void* InKey) {
    return JsonSerializer::SerializeProperty(InMap.KeyProperty, const_cast<void*>(InKey));
}

static void WriteMapKey(const MapProperty& InMap, void* OutKey, const nlohmann::json& InValue) {
    JsonSerializer::DeserializeProperty(InMap.KeyProperty, OutKey, InValue);
}

static uint64_t MapEntryOffset(uint64_t InMapOffset, MapProperty* InMap, const void* InKey) {
    const nlohmann::json key = ReadMapKey(*InMap, InKey);
    const String keyText = key.dump();
    Array<ContainerElementAddress>& addresses = GetElementAddresses();
    int32_t slot = -1;
    for (int32_t i = 0; i < addresses.Size() && slot < 0; i++) {
        const ContainerElementAddress& address = addresses[i];
        if (address.ContainerOffset == InMapOffset && address.Map == InMap && address.KeyText == keyText) {
            slot = i;
        }
    }
    if (slot < 0) {
        ContainerElementAddress address;
        address.ContainerOffset = InMapOffset;
        address.Map = InMap;
        address.KeyText = keyText;
        address.Key = InMap->MakeKey();
        address.PendingKey = InMap->MakeKey();
        addresses.Add(address);
        slot = addresses.Last();
    }
    WriteMapKey(*InMap, addresses[slot].Key.get(), key);
    WriteMapKey(*InMap, addresses[slot].PendingKey.get(), key);
    return ElementOffset(slot);
}

static char* ResolveBase(const WeakObjectPtr<Object>& InObject, uint64_t InOffset) {
    if (!(InOffset & s_ElementBit)) {
        Object* object = InObject.Get();
        return object ? (char*)object + InOffset : nullptr;
    }
    const ContainerElementAddress& address = GetElementAddresses()[ElementSlot(InOffset)];
    if (address.Resolve) {
        Object* object = InObject.Get();
        char* external = object ? (char*)address.Resolve(object) : nullptr;
        return external ? external + (InOffset & s_ElementMemberMask) : nullptr;
    }
    char* container = ResolveBase(InObject, address.ContainerOffset);
    if (!container) {
        return nullptr;
    }
    const uint64_t member = InOffset & s_ElementMemberMask;
    if (address.Array) {
        if (address.Index >= (int32_t)address.Array->GetSize(container)) {
            return nullptr;
        }
        return (char*)address.Array->GetElementPtr(container, (size_t)address.Index) + member;
    }
    char* value = (char*)address.Map->Find(container, address.Key.get());
    if (!value) {
        return nullptr;
    }
    return (InOffset & s_MapKeyBit) ? (char*)address.PendingKey.get() + member : value + member;
}

uint64_t DetailsCustomization::MakeExternalOffset(const String& InKey, std::function<void*(Object*)> InResolve) {
    Array<ContainerElementAddress>& addresses = GetElementAddresses();
    for (int32_t i = 0; i < addresses.Size(); i++) {
        if (addresses[i].Resolve && addresses[i].ExternalKey == InKey) {
            addresses[i].Resolve = std::move(InResolve);
            return ElementOffset(i);
        }
    }
    ContainerElementAddress address;
    address.ExternalKey = InKey;
    address.Resolve = std::move(InResolve);
    addresses.Add(address);
    return ElementOffset(addresses.Last());
}

static bool IsOverridablePath(const String& InPath) {
    return InPath.find('[') == String::npos;
}

static String OverridablePath(const String& InPath) {
    return InPath.substr(0, InPath.find('['));
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

static void BuildNumberEditor(UINode& InHost, const WeakObjectPtr<Object>& InObject, uint64_t InOffset, Property* InProperty,
                              const DetailsEditHandler& InOnEdited) {
    UIDragNumber* drag = InHost.Add<UIDragNumber>();
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
        drag->Set = [InObject, InOffset, intProperty, InOnEdited](double InValue) {
            if (char* base = ResolveBase(InObject, InOffset)) {
                InOnEdited.Record();
                WriteIntValue(*intProperty, base, InValue);
                InOnEdited();
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
        drag->Set = [InObject, InOffset, floatProperty, InOnEdited](double InValue) {
            if (char* base = ResolveBase(InObject, InOffset)) {
                InOnEdited.Record();
                if (floatProperty->IsDouble) {
                    *(double*)base = InValue;
                } else {
                    *(float*)base = (float)InValue;
                }
                InOnEdited();
            }
        };
    }
    if (InProperty->HasRange) {
        drag->MinValue = std::max(drag->MinValue, InProperty->RangeMin);
        drag->MaxValue = InProperty->RangeMax;
        drag->Slider = true;
    }
}

static int32_t VectorComponentCount(const String& InTypename) {
    return InTypename == "Vec2" ? 2 : InTypename == "Vec3" ? 3 : InTypename == "Vec4" ? 4 : 0;
}

static void BuildVectorEditor(UINode& InHost, const WeakObjectPtr<Object>& InObject, uint64_t InOffset, int32_t InComponents,
                              const DetailsEditHandler& InOnEdited) {
    static const char* s_Names[4] = { "X", "Y", "Z", "W" };
    const Vec4 colors[4] = { EditorStyle::TransformX, EditorStyle::TransformY, EditorStyle::TransformZ, EditorStyle::TextDim };

    UIHStack* stack = InHost.Add<UIHStack>();
    stack->Fill();
    stack->Gap = 2.0f;
    for (int32_t component = 0; component < InComponents; component++) {
        UIDragNumber* drag = stack->Add<UIDragNumber>();
        drag->Size = { UIValue::Rel(1.0f / (float)InComponents), 1.0_rel };
        drag->Prefix = s_Names[component];
        drag->PrefixColor = colors[component];
        drag->Get = [InObject, InOffset, component]() -> double {
            char* base = ResolveBase(InObject, InOffset);
            return base ? (double)reinterpret_cast<float*>(base)[component] : 0.0;
        };
        drag->Set = [InObject, InOffset, component, InOnEdited](double InValue) {
            if (char* base = ResolveBase(InObject, InOffset)) {
                InOnEdited.Record();
                reinterpret_cast<float*>(base)[component] = (float)InValue;
                InOnEdited();
            }
        };
    }
}

static void BuildColorEditor(UINode& InHost, const WeakObjectPtr<Object>& InObject, uint64_t InOffset,
                             const String& InTitle, const DetailsEditHandler& InOnEdited) {
    UIColorSwatch* swatch = InHost.Add<UIColorSwatch>();
    swatch->Fill();
    swatch->Title = InTitle.empty() ? String("Color") : InTitle;
    swatch->Get = [InObject, InOffset]() -> Color {
        char* base = ResolveBase(InObject, InOffset);
        return base ? *(Color*)base : Color(0.0f);
    };
    swatch->Set = [InObject, InOffset, InOnEdited](const Color& InValue) {
        if (char* base = ResolveBase(InObject, InOffset)) {
            InOnEdited.Record();
            *(Color*)base = InValue;
            InOnEdited();
        }
    };
}

static bool IsInlinePair(const String& InTypename) {
    const Array<Property*> members = Property::GetTypeProperties(InTypename);
    if (members.Size() != 2) {
        return false;
    }
    const bool named = (members[0]->Name == "Min" && members[1]->Name == "Max")
                    || (members[0]->Name == "Time" && members[1]->Name == "Value");
    const auto isInline = [](Property* InMember) {
        StructProperty* structMember = Cast<StructProperty>(InMember);
        return Cast<FloatProperty>(InMember) || (structMember && structMember->InnerStructTypename == "Color");
    };
    return named && isInline(members[0]) && isInline(members[1]);
}

static void BuildInlinePairEditor(UINode& InHost, const WeakObjectPtr<Object>& InObject, uint64_t InOffset, const String& InTypename,
                                  const String& InTitle, const DetailsEditHandler& InOnEdited) {
    UIHStack* stack = InHost.Add<UIHStack>();
    stack->Fill();
    stack->Gap = 2.0f;
    for (Property* member : Property::GetTypeProperties(InTypename)) {
        const uint64_t offset = InOffset + member->Offset;
        if (!Cast<FloatProperty>(member)) {
            UINode* cell = stack->Add<UINode>();
            cell->Size = { UIValue::Rel(0.5f), 1.0_rel };
            BuildColorEditor(*cell, InObject, offset, InTitle + " " + member->Name, InOnEdited);
            continue;
        }
        UIDragNumber* drag = stack->Add<UIDragNumber>();
        drag->Size = { UIValue::Rel(0.5f), 1.0_rel };
        drag->Get = [InObject, offset]() -> double {
            char* base = ResolveBase(InObject, offset);
            return base ? (double)*reinterpret_cast<float*>(base) : 0.0;
        };
        drag->Set = [InObject, offset, InOnEdited](double InValue) {
            if (char* base = ResolveBase(InObject, offset)) {
                InOnEdited.Record();
                *reinterpret_cast<float*>(base) = (float)InValue;
                InOnEdited();
            }
        };
    }
}

static void BuildBoolEditor(UINode& InHost, const WeakObjectPtr<Object>& InObject, uint64_t InOffset,
                            const DetailsEditHandler& InOnEdited) {
    UICheckbox* checkbox = InHost.Add<UICheckbox>();
    checkbox->Anchor = checkbox->Pivot = Vec2(0.0f, 0.5f);
    checkbox->Position = Vec2(2.0f, 0.0f);
    checkbox->Bind = [checkbox, InObject, InOffset] {
        if (char* base = ResolveBase(InObject, InOffset)) {
            checkbox->IsOn = *(bool*)base;
        }
    };
    checkbox->Changed = [InObject, InOffset, InOnEdited](bool InValue) {
        if (char* base = ResolveBase(InObject, InOffset)) {
            InOnEdited.Record();
            *(bool*)base = InValue;
            InOnEdited();
        }
    };
}

static void BuildStringEditor(UINode& InHost, const WeakObjectPtr<Object>& InObject, uint64_t InOffset,
                              const DetailsEditHandler& InOnEdited) {
    UITextArea* field = InHost.Add<UITextArea>();
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
    const auto commit = [InObject, InOffset, InOnEdited](const String& InText) {
        if (char* base = ResolveBase(InObject, InOffset)) {
            InOnEdited.Record();
            *(String*)base = InText;
            InOnEdited();
        }
    };
    field->Submitted = commit;
    field->FocusLost = [field, commit] { commit(field->Text); };
}

static void BuildEnumEditor(UINode& InHost, const WeakObjectPtr<Object>& InObject, uint64_t InOffset, EnumProperty* InProperty,
                            const DetailsEditHandler& InOnEdited) {
    const auto read = [InObject, InOffset, InProperty]() -> int64_t {
        int64_t value = 0;
        if (char* base = ResolveBase(InObject, InOffset)) {
            std::memcpy(&value, base, InProperty->ByteSize);
        }
        return value;
    };

    UIDropdown* dropdown = InHost.Add<UIDropdown>();
    dropdown->Fill();
    dropdown->GetSelectedLabel = [read, InProperty] {
        return Enum(InProperty->InnerEnumTypename).ConvertValueToString(read());
    };
    dropdown->GetOptions = [InProperty] {
        Array<UIDropdownOption> options;
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
    dropdown->SelectionChanged = [InObject, InOffset, InProperty, InOnEdited](int32_t InIndex) {
        const Array<Enum::EnumValue>& values = Enum(InProperty->InnerEnumTypename).GetValues();
        if (InIndex < 0 || InIndex >= values.Size()) {
            return;
        }
        if (char* base = ResolveBase(InObject, InOffset)) {
            InOnEdited.Record();
            std::memcpy(base, &values[InIndex].Value, InProperty->ByteSize);
            InOnEdited();
        }
    };
}

static Array<Class> GetClassOptions(const Class& InBaseClass) {
    const bool allowEditorClasses = InBaseClass.GetModuleName() == "Editor";
    Array<Class> classes;
    for (const Class& candidate : Class::GetSubclassesOf(InBaseClass)) {
        if (candidate.IsDefaultConstructable() && (allowEditorClasses || candidate.GetModuleName() != "Editor")) {
            classes.Add(candidate);
        }
    }
    for (Asset* asset : AssetManager::Get().GetAssetsOfClass(Blueprint::StaticClass())) {
        const Class blueprint = Class::FromBlueprint(asset->GetId());
        if (blueprint.IsSubclassOf(InBaseClass)) {
            classes.Add(blueprint);
        }
    }
    classes.Sort([](const Class& InA, const Class& InB) { return InA.GetDisplayName() < InB.GetDisplayName(); });
    return classes;
}

static String ClassLabel(const Class& InClass) {
    return InClass == Class::None ? String("None") : InClass.GetDisplayName();
}

static void BuildClassEditor(UINode& InHost, const WeakObjectPtr<Object>& InObject, uint64_t InOffset, SubclassOfProperty* InProperty,
                             const DetailsEditHandler& InOnEdited) {
    const auto read = [InObject, InOffset, InProperty]() -> Class {
        char* base = ResolveBase(InObject, InOffset);
        return base ? InProperty->Read(base) : Class::None;
    };

    UIDropdown* dropdown = InHost.Add<UIDropdown>();
    dropdown->Fill();
    dropdown->SearchPlaceholder = "Search classes";
    dropdown->GetSelectedLabel = [read] { return ClassLabel(read()); };
    dropdown->GetSelectedIcon = [read]() -> VectorImage* {
        return read().IsBlueprint() ? EditorIcons::Node() : nullptr;
    };
    dropdown->GetOptions = [InProperty] {
        Array<UIDropdownOption> options;
        options.Add(UIDropdownOption("None"));
        for (const Class& option : GetClassOptions(InProperty->BaseClass)) {
            UIDropdownOption entry(option.GetDisplayName());
            entry.Icon = option.IsBlueprint() ? EditorIcons::Node() : nullptr;
            options.Add(entry);
        }
        return options;
    };
    dropdown->GetSelectedIndex = [read, InProperty]() -> int32_t {
        const Class current = read();
        if (current == Class::None) {
            return 0;
        }
        const int32_t index = GetClassOptions(InProperty->BaseClass).IndexOf(current);
        return index < 0 ? -1 : index + 1;
    };
    dropdown->SelectionChanged = [InObject, InOffset, InProperty, InOnEdited](int32_t InIndex) {
        const Array<Class> options = GetClassOptions(InProperty->BaseClass);
        if (InIndex < 0 || InIndex > options.Size()) {
            return;
        }
        if (char* base = ResolveBase(InObject, InOffset)) {
            InOnEdited.Record();
            InProperty->Write(base, InIndex == 0 ? Class::None : options[InIndex - 1]);
            InOnEdited();
        }
    };
}

static void BuildAssetSlot(UINode& InHost, DetailsTab& InTab, const WeakObjectPtr<Object>& InObject, uint64_t InOffset,
                           const Class& InAssetClass, bool InIsWeak, const DetailsEditHandler& InOnEdited) {
    UIAssetSlot* slot = InHost.Add<UIAssetSlot>();
    slot->Fill();
    slot->AssetClass = InAssetClass;
    slot->Thumbnails = &InTab.GetThumbnails();
    slot->GetAsset = [InObject, InOffset, InIsWeak]() -> Asset* {
        char* base = ResolveBase(InObject, InOffset);
        return base ? Cast<Asset>(ReadObjectPtr(base, InIsWeak)) : nullptr;
    };
    slot->SetAsset = [InObject, InOffset, InIsWeak, InOnEdited](Asset* InAsset) {
        if (char* base = ResolveBase(InObject, InOffset)) {
            InOnEdited.Record();
            WriteObjectPtr(base, InIsWeak, InAsset);
            InOnEdited();
        }
    };
    slot->Build();
}

static void BuildAssetRow(DetailsRow& InRow, DetailsTab& InTab, const WeakObjectPtr<Object>& InObject, uint64_t InOffset,
                          const Class& InAssetClass, bool InIsWeak, const DetailsEditHandler& InOnEdited) {
    InRow.Height = AssetRowHeight;
    BuildAssetSlot(*InRow.GetValueHost(), InTab, InObject, InOffset, InAssetClass, InIsWeak, InOnEdited);
}

static const Class* FindAssetReferenceClass(Property* InProperty, bool& OutIsWeak) {
    if (WeakObjectPtrProperty* weak = Cast<WeakObjectPtrProperty>(InProperty)) {
        OutIsWeak = true;
        return weak->InnerClass.IsSubclassOf(Asset::StaticClass()) ? &weak->InnerClass : nullptr;
    }
    if (SharedObjectPtrProperty* shared = Cast<SharedObjectPtrProperty>(InProperty)) {
        OutIsWeak = false;
        return shared->InnerClass.IsSubclassOf(Asset::StaticClass()) ? &shared->InnerClass : nullptr;
    }
    return nullptr;
}

static bool IsEditableArrayElement(Property* InProperty) {
    bool isWeak = false;
    if (Cast<WeakObjectPtrProperty>(InProperty) || Cast<SharedObjectPtrProperty>(InProperty)) {
        return FindAssetReferenceClass(InProperty, isWeak) != nullptr;
    }
    return Cast<IntProperty>(InProperty) || Cast<FloatProperty>(InProperty) || Cast<BoolProperty>(InProperty)
        || Cast<StringProperty>(InProperty) || Cast<EnumProperty>(InProperty) || Cast<StructProperty>(InProperty)
        || Cast<SubclassOfProperty>(InProperty);
}

static UIButton& AddHeaderIconButton(UINode& InParent, VectorImage* InIcon, const String& InTooltip, float InRightInset,
                                     std::function<void()> InAction) {
    UIButton* button = InParent.Add<UIButton>();
    button->Anchor = button->Pivot = Vec2(1.0f, 0.5f);
    button->Position = Vec2(-InRightInset, 0.0f);
    button->Size = Vec2(20.0f, 20.0f);
    button->NormalColor = Vec4(0.0f);
    button->HoverColor = EditorStyle::TabHover;
    button->PressedColor = EditorStyle::ButtonPressed;
    button->Clicked = std::move(InAction);

    UISvg* icon = button->Add<UISvg>();
    icon->Center(Vec2(12.0f, 12.0f));
    icon->Tint = EditorStyle::TextDim;
    icon->Image = InIcon;

    button->Add<UITooltip>()->Text = InTooltip;
    return *button;
}

static void EditContainer(const WeakObjectPtr<DetailsTab>& InTab, const WeakObjectPtr<Object>& InObject, uint64_t InOffset,
                          Property* InRootProperty, const String& InPath, const String& InTitle, const std::function<void(char*)>& InEdit) {
    DetailsTab* tab = InTab.Get();
    Object* object = InObject.Get();
    char* container = ResolveBase(InObject, InOffset);
    if (!tab || !object || !container) {
        return;
    }
    tab->RecordEdit(InTitle, object);
    InEdit(container);
    InRootProperty->NotifyChanged(object);
    DetailsCustomization::NotifyPropertyEdited(tab, object, InPath);
    if (MajorTab* major = tab->GetMajorTab()) {
        major->EndTransaction();
    }
    tab->MarkDirty();
}

static String Plural(size_t InCount, const String& InSingular, const String& InPlural) {
    return std::to_string(InCount) + " " + (InCount == 1 ? InSingular : InPlural);
}

static void AddContainerHeader(DetailsCategory& InCategory, std::function<String()> InSummary, const String& InAddTooltip,
                               std::function<void()> InAdd, const String& InClearTooltip, std::function<void()> InClear) {
    UINode* header = InCategory.AddHeaderValue();
    UILabel& summary = AddValueLabel(*header, "");
    summary.Size = { 1.0_rel - 50.0_px, 1.0_rel };
    summary.Bind = [summary = &summary, InSummary] { summary->Text = InSummary(); };
    AddHeaderIconButton(*header, EditorIcons::Add(), InAddTooltip, 24.0f, std::move(InAdd));
    AddHeaderIconButton(*header, EditorIcons::Delete(), InClearTooltip, 0.0f, std::move(InClear));
}

static void AddMenuButton(UINode& InHost, const String& InTooltip, std::function<void(UIMenuModel&)> InBuildMenu) {
    UIButton& button = AddHeaderIconButton(InHost, EditorIcons::More(), InTooltip, 0.0f, nullptr);
    button.Clicked = [button = &button, InBuildMenu] {
        UIMenuModel menu;
        InBuildMenu(menu);
        UIContextMenu::OpenUnder(*button, menu);
    };
}

static String MapKeyLabel(const MapProperty& InMap, const String& InKeyText) {
    const nlohmann::json key = nlohmann::json::parse(InKeyText, nullptr, false);
    if (key.is_null() || key.is_discarded()) {
        return "None";
    }
    if (!key.is_string()) {
        return key.dump();
    }
    const String text = key.get<String>();
    if (Cast<SubclassOfProperty>(InMap.KeyProperty)) {
        return ClassLabel(Class(text));
    }
    return text.empty() ? String("(empty)") : text;
}

static Array<MapProperty::Entry> SortedMapEntries(const MapProperty& InMap, char* InData) {
    Array<MapProperty::Entry> entries;
    InMap.GetEntries(InData, entries);
    Array<std::pair<nlohmann::json, MapProperty::Entry>> keyed;
    for (const MapProperty::Entry& entry : entries) {
        keyed.Add({ ReadMapKey(InMap, entry.Key), entry });
    }
    keyed.Sort([](const auto& InA, const auto& InB) { return InA.first < InB.first; });
    Array<MapProperty::Entry> sorted;
    for (const auto& [key, entry] : keyed) {
        sorted.Add(entry);
    }
    return sorted;
}

static bool MakeUniqueMapKey(const MapProperty& InMap, char* InData, void* InOutKey) {
    const auto isFree = [&] { return InMap.Find(InData, InOutKey) == nullptr; };
    Property* keyProperty = InMap.KeyProperty;
    if (Cast<UUIDProperty>(keyProperty)) {
        *(UUID*)InOutKey = UUID::Generate();
        return isFree();
    }
    if (EnumProperty* enumProperty = Cast<EnumProperty>(keyProperty)) {
        for (const Enum::EnumValue& value : Enum(enumProperty->InnerEnumTypename).GetValues()) {
            std::memcpy(InOutKey, &value.Value, enumProperty->ByteSize);
            if (isFree()) {
                return true;
            }
        }
        return false;
    }
    if (isFree()) {
        return true;
    }
    if (Cast<StringProperty>(keyProperty)) {
        const String base = ((String*)InOutKey)->empty() ? String("Key") : *(String*)InOutKey;
        for (size_t i = 1; i <= InMap.GetSize(InData) + 1; i++) {
            *(String*)InOutKey = base + " " + std::to_string(i);
            if (isFree()) {
                return true;
            }
        }
    } else if (IntProperty* intProperty = Cast<IntProperty>(keyProperty)) {
        for (size_t i = 0; i <= InMap.GetSize(InData) + 1; i++) {
            WriteIntValue(*intProperty, InOutKey, (double)i);
            if (isFree()) {
                return true;
            }
        }
    } else if (Cast<BoolProperty>(keyProperty)) {
        *(bool*)InOutKey = !*(bool*)InOutKey;
        return isFree();
    } else if (SubclassOfProperty* subclassProperty = Cast<SubclassOfProperty>(keyProperty)) {
        for (const Class& option : GetClassOptions(subclassProperty->BaseClass)) {
            *(Class*)InOutKey = option;
            if (isFree()) {
                return true;
            }
        }
    }
    return false;
}

static bool InsertMapEntry(const MapProperty& InMap, char* InData, const void* InSourceKey) {
    const std::shared_ptr<void> key = InMap.MakeKey();
    if (InSourceKey) {
        WriteMapKey(InMap, key.get(), ReadMapKey(InMap, InSourceKey));
    }
    if (!MakeUniqueMapKey(InMap, InData, key.get())) {
        AE_WARN("Every possible key of '{0}' is already in use", InMap.Name);
        return false;
    }
    void* value = InMap.FindOrAdd(InData, key.get());
    if (void* source = InSourceKey ? InMap.Find(InData, InSourceKey) : nullptr) {
        JsonSerializer::DeserializeProperty(InMap.ValueProperty, value, JsonSerializer::SerializeProperty(InMap.ValueProperty, source));
    }
    return true;
}

static void CommitMapKey(const WeakObjectPtr<DetailsTab>& InTab, const WeakObjectPtr<Object>& InObject, uint64_t InEntryOffset,
                         Property* InRootProperty, const String& InMapPath) {
    ContainerElementAddress& address = GetElementAddresses()[ElementSlot(InEntryOffset)];
    DetailsTab* tab = InTab.Get();
    Object* object = InObject.Get();
    char* data = ResolveBase(InObject, address.ContainerOffset);
    MapProperty* map = address.Map;
    if (!tab || !object || !data || !map) {
        return;
    }
    const nlohmann::json current = ReadMapKey(*map, address.Key.get());
    const nlohmann::json pending = ReadMapKey(*map, address.PendingKey.get());
    if (pending == current) {
        return;
    }
    if (map->Find(data, address.PendingKey.get())) {
        AE_WARN("'{0}' already has an entry for {1}", DetailsCustomization::PrettyPropertyPath(OverridablePath(InMapPath)), pending.dump());
        WriteMapKey(*map, address.PendingKey.get(), current);
        return;
    }
    tab->RecordEdit("Edit " + DetailsCustomization::PrettyPropertyPath(OverridablePath(InMapPath)) + " Key", object);
    map->RenameKey(data, address.Key.get(), address.PendingKey.get());
    WriteMapKey(*map, address.Key.get(), pending);
    address.KeyText = pending.dump();
    InRootProperty->NotifyChanged(object);
    DetailsCustomization::NotifyPropertyEdited(tab, object, InMapPath);
}

static void MoveArrayElement(ArrayProperty& InArray, char* InData, int32_t InFrom, int32_t InTo) {
    for (int32_t i = InFrom; i > InTo; i--) {
        InArray.Swap(InData, (size_t)i, (size_t)i - 1);
    }
    for (int32_t i = InFrom; i < InTo; i++) {
        InArray.Swap(InData, (size_t)i, (size_t)i + 1);
    }
}

Array<Property*> DetailsCustomization::GetInnerProperties(Property* InProperty) {
    StructProperty* structProperty = Cast<StructProperty>(InProperty);
    if (!structProperty || structProperty->InnerStructTypename == "Color"
        || VectorComponentCount(structProperty->InnerStructTypename) > 0 || IsInlinePair(structProperty->InnerStructTypename)) {
        return Array<Property*>();
    }
    return Property::GetTypeProperties(structProperty->InnerStructTypename);
}

bool DetailsCustomization::BuildValueEditor(UINode& InHost, const WeakObjectPtr<Object>& InObject, uint64_t InOffset,
                                            Property* InProperty, const DetailsEditHandler& InOnEdited, const String& InTitle) {
    if (Cast<IntProperty>(InProperty) || Cast<FloatProperty>(InProperty)) {
        BuildNumberEditor(InHost, InObject, InOffset, InProperty, InOnEdited);
    } else if (Cast<BoolProperty>(InProperty)) {
        BuildBoolEditor(InHost, InObject, InOffset, InOnEdited);
    } else if (Cast<StringProperty>(InProperty)) {
        BuildStringEditor(InHost, InObject, InOffset, InOnEdited);
    } else if (EnumProperty* enumProperty = Cast<EnumProperty>(InProperty)) {
        BuildEnumEditor(InHost, InObject, InOffset, enumProperty, InOnEdited);
    } else if (SubclassOfProperty* subclassProperty = Cast<SubclassOfProperty>(InProperty)) {
        BuildClassEditor(InHost, InObject, InOffset, subclassProperty, InOnEdited);
    } else if (StructProperty* structProperty = Cast<StructProperty>(InProperty)) {
        if (const int32_t components = VectorComponentCount(structProperty->InnerStructTypename)) {
            BuildVectorEditor(InHost, InObject, InOffset, components, InOnEdited);
        } else if (structProperty->InnerStructTypename == "Color") {
            BuildColorEditor(InHost, InObject, InOffset, InTitle, InOnEdited);
        } else if (IsInlinePair(structProperty->InnerStructTypename)) {
            BuildInlinePairEditor(InHost, InObject, InOffset, structProperty->InnerStructTypename, InTitle, InOnEdited);
        } else {
            return false;
        }
    } else {
        return false;
    }
    return true;
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

DetailsEditHandler DetailsCustomization::MakeEditHandler(const WeakObjectPtr<Object>& InObject, Property* InRootProperty, DetailsTab* InTab,
                                                         const String& InPropertyPath) {
    const WeakObjectPtr<DetailsTab> tab = InTab;
    const String path = !InPropertyPath.empty() ? InPropertyPath : InRootProperty ? InRootProperty->Name : String();
    DetailsEditHandler handler;
    handler.BeginEdit = [InObject, path, tab] {
        if (!path.empty() && tab.Get()) {
            tab.Get()->RecordEdit("Edit " + PrettyPropertyPath(path), InObject.Get());
        }
    };
    handler.CommitEdit = [InObject, InRootProperty, path, tab] {
        Object* object = InObject.Get();
        if (!object || !InRootProperty) {
            return;
        }
        InRootProperty->NotifyChanged(object);
        NotifyPropertyEdited(tab.Get(), object, path);
    };
    return handler;
}

void DetailsCustomization::NotifyPropertyEdited(MajorTab* InTab, Object* InObject, const String& InPropertyName) {
    if (!InObject) {
        return;
    }
    const String path = OverridablePath(InPropertyName);
    if (Node* node = Cast<Node>(InObject)) {
        node->MarkPropertyOverridden(path);
    }
    if (InTab) {
        InTab->OnObjectEdited(InObject);
        InTab->OnPropertyEdited(InObject, PropertyPath::GetRootName(path));
    }
}

void DetailsCustomization::NotifyPropertyEdited(DetailsTab* InTab, Object* InObject, const String& InPropertyName) {
    NotifyPropertyEdited(InTab ? InTab->GetMajorTab() : nullptr, InObject, InPropertyName);
    if (!InTab || !InObject) {
        return;
    }
    DetailsCustomization* customization = FindFor(InObject->GetClass());
    if (customization && customization->RebuildsOnEdit(PropertyPath::GetRootName(OverridablePath(InPropertyName)))) {
        InTab->MarkDirty();
    }
}

static bool BindOverrideActions(const WeakObjectPtr<Object>& InObject, const String& InPropertyPath, DetailsTab* InTab,
                                std::function<bool()>& OutIsOverridden, std::function<void()>& OutReset) {
    if (!Cast<Node>(InObject.Get())) {
        return false;
    }
    OutIsOverridden = [InObject, InPropertyPath] {
        Node* node = Cast<Node>(InObject.Get());
        return node && node->IsPropertyOverridden(InPropertyPath);
    };
    OutReset = [InObject, InPropertyPath, tab = WeakObjectPtr<DetailsTab>(InTab)] {
        Node* node = Cast<Node>(InObject.Get());
        if (!node || !tab.Get()) {
            return;
        }
        tab.Get()->RecordEdit("Reset " + DetailsCustomization::PrettyPropertyPath(InPropertyPath), node);
        node->ResetPropertyToDefault(InPropertyPath);
        if (MajorTab* major = tab.Get()->GetMajorTab()) {
            major->OnObjectEdited(node);
            major->OnPropertyEdited(node, PropertyPath::GetRootName(InPropertyPath));
        }
    };
    return true;
}

void DetailsCustomization::BindOverride(DetailsRow& InRow, const WeakObjectPtr<Object>& InObject, const String& InPropertyPath) {
    BindOverrideActions(InObject, InPropertyPath, InRow.OwnerTab, InRow.IsOverridden, InRow.ResetAction);
}

/** A struct with a bool member named Enabled shows it as a checkbox in its category header, and dims
 *  its other members while it is off. */
static Property* FindHeaderToggle(const Array<Property*>& InMembers) {
    for (Property* member : InMembers) {
        if (member->Name == "Enabled" && Cast<BoolProperty>(member)) {
            return member;
        }
    }
    return nullptr;
}

void DetailsCustomization::AddPropertyRow(UINode& InParent, DetailsTab& InTab, const WeakObjectPtr<Object>& InObject,
                                          uint64_t InBaseOffset, Property* InProperty, int32_t InDepth,
                                          Property* InRootProperty, const String& InLabel, const String& InParentPath) {
    const uint64_t offset = InBaseOffset + InProperty->Offset;
    Property* root = InRootProperty ? InRootProperty : InProperty;
    const String label = InLabel.empty() ? PrettyPropertyName(InProperty->Name) : InLabel;
    const String path = !InParentPath.empty() ? InParentPath + "." + InProperty->Name
                      : InRootProperty ? InRootProperty->Name : InProperty->Name;

    const Array<Property*> inner = GetInnerProperties(InProperty);
    if (!inner.IsEmpty()) {
        AddStructCategory(InParent, InTab, InObject, offset, inner, InDepth, root, label, path).SetTooltip(InProperty->Comment);
        return;
    }

    ArrayProperty* editableArray = Cast<ArrayProperty>(InProperty);
    if (editableArray && IsEditableArrayElement(editableArray->InnerProperty)) {
        AddArrayRows(InParent, InTab, InObject, offset, editableArray, InDepth, root, label, path).SetTooltip(InProperty->Comment);
        return;
    }

    MapProperty* editableMap = Cast<MapProperty>(InProperty);
    if (editableMap && IsEditableArrayElement(editableMap->ValueProperty)) {
        AddMapRows(InParent, InTab, InObject, offset, editableMap, InDepth, root, label, path).SetTooltip(InProperty->Comment);
        return;
    }

    DetailsRow& row = AddRow(InParent, InTab, label, InDepth);
    row.SetTooltip(InProperty->Comment);
    if (DelegateProperty* delegateProperty = Cast<DelegateProperty>(InProperty)) {
        UILabel& summary = AddValueLabel(*row.GetValueHost(), "");
        summary.Bind = [summary = &summary, InObject, offset, delegateProperty] {
            char* base = ResolveBase(InObject, offset);
            const int32_t count = base ? delegateProperty->GetBindingCount(base) : 0;
            summary->Text = "void(" + delegateProperty->Signature + ")   " + Plural((size_t)count, "listener", "listeners");
        };
        return;
    }
    if (IsOverridablePath(path)) {
        BindOverride(row, InObject, path);
    }
    const DetailsEditHandler onEdited = MakeEditHandler(InObject, root, &InTab, path);

    if (BuildValueEditor(*row.GetValueHost(), InObject, offset, InProperty, onEdited, label)) {
        return;
    }

    if (StructProperty* structProperty = Cast<StructProperty>(InProperty)) {
        AddValueLabel(*row.GetValueHost(), structProperty->InnerStructTypename);
    } else if (SharedObjectPtrProperty* sharedProperty = Cast<SharedObjectPtrProperty>(InProperty)) {
        if (sharedProperty->InnerClass.IsSubclassOf(Asset::StaticClass())) {
            BuildAssetRow(row, InTab, InObject, offset, sharedProperty->InnerClass, false, onEdited);
        } else {
            AddValueLabel(*row.GetValueHost(), sharedProperty->InnerClass.Name);
        }
    } else if (WeakObjectPtrProperty* weakProperty = Cast<WeakObjectPtrProperty>(InProperty)) {
        if (weakProperty->InnerClass.IsSubclassOf(Asset::StaticClass())) {
            BuildAssetRow(row, InTab, InObject, offset, weakProperty->InnerClass, true, onEdited);
        } else {
            AddValueLabel(*row.GetValueHost(), weakProperty->InnerClass.Name);
        }
    } else if (ArrayProperty* arrayProperty = Cast<ArrayProperty>(InProperty)) {
        UILabel& label = AddValueLabel(*row.GetValueHost(), "");
        label.Bind = [label = &label, InObject, offset, arrayProperty] {
            char* base = ResolveBase(InObject, offset);
            label->Text = base ? Plural(arrayProperty->GetSize(base), "element", "elements") : "";
        };
    } else if (MapProperty* mapProperty = Cast<MapProperty>(InProperty)) {
        UILabel& label = AddValueLabel(*row.GetValueHost(), "");
        label.Bind = [label = &label, InObject, offset, mapProperty] {
            char* base = ResolveBase(InObject, offset);
            label->Text = base ? Plural(mapProperty->GetSize(base), "entry", "entries") : "";
        };
    } else {
        AddValueLabel(*row.GetValueHost(), "Unsupported");
    }
}

/** The key type of a curve or gradient, found through its Keys array. Empty for any other struct. */
static String FindKeyType(const Array<Property*>& InMembers) {
    for (Property* member : InMembers) {
        ArrayProperty* keys = Cast<ArrayProperty>(member);
        StructProperty* key = keys && member->Name == "Keys" ? Cast<StructProperty>(keys->InnerProperty) : nullptr;
        if (key && (key->InnerStructTypename == "ParticleCurveKey" || key->InnerStructTypename == "ParticleGradientKey")) {
            return key->InnerStructTypename;
        }
    }
    return String();
}

static void BuildKeyGraph(DetailsRow& InRow, const WeakObjectPtr<Object>& InObject, uint64_t InOffset, bool InGradient,
                          const DetailsEditHandler& InOnEdited) {
    const auto resolve = [InObject, InOffset] { return ResolveBase(InObject, InOffset); };
    const auto beginChange = [InOnEdited] { InOnEdited.Record(); };
    const auto changed = [InOnEdited] { InOnEdited(); };
    if (InGradient) {
        InRow.Height = 36.0f;
        UIGradientEditor* editor = InRow.GetValueHost()->Add<UIGradientEditor>();
        editor->Fill();
        editor->GetGradient = [resolve] { return reinterpret_cast<ParticleGradient*>(resolve()); };
        editor->BeginChange = beginChange;
        editor->Changed = changed;
        return;
    }
    InRow.Height = 80.0f;
    UICurveEditor* editor = InRow.GetValueHost()->Add<UICurveEditor>();
    editor->Fill();
    editor->GetCurve = [resolve] { return reinterpret_cast<ParticleCurve*>(resolve()); };
    editor->BeginChange = beginChange;
    editor->Changed = changed;
}

DetailsCategory& DetailsCustomization::AddStructCategory(UINode& InParent, DetailsTab& InTab, const WeakObjectPtr<Object>& InObject,
                                                         uint64_t InOffset, const Array<Property*>& InMembers, int32_t InDepth,
                                                         Property* InRootProperty, const String& InLabel, const String& InPath,
                                                         const std::function<void(DetailsCategory&)>& InBeforeMembers) {
    DetailsCategory& category = AddCategory(InParent, InTab, InLabel, InDepth + 1);
    if (InBeforeMembers) {
        InBeforeMembers(category);
    }
    Property* toggle = FindHeaderToggle(InMembers);
    if (toggle) {
        const String togglePath = InPath + "." + toggle->Name;
        const uint64_t toggleOffset = InOffset + toggle->Offset;
        BuildValueEditor(*category.AddHeaderEditor(20.0f), InObject, toggleOffset, toggle,
                         MakeEditHandler(InObject, InRootProperty, &InTab, togglePath));
        category.IsActive = [InObject, toggleOffset] {
            char* base = ResolveBase(InObject, toggleOffset);
            return !base || *(bool*)base;
        };
        if (IsOverridablePath(togglePath)) {
            BindOverrideActions(InObject, togglePath, &InTab, category.IsOverridden, category.ResetAction);
        }
    }
    const String keyType = FindKeyType(InMembers);
    if (!keyType.empty()) {
        const bool gradient = keyType == "ParticleGradientKey";
        BuildKeyGraph(AddRow(*category.GetBody(), InTab, gradient ? "Gradient" : "Curve", InDepth + 1), InObject, InOffset, gradient,
                      MakeEditHandler(InObject, InRootProperty, &InTab, InPath));
    }
    for (Property* member : InMembers) {
        if (member != toggle) {
            AddPropertyRow(*category.GetBody(), InTab, InObject, InOffset, member, InDepth + 1, InRootProperty, String(), InPath);
        }
    }
    return category;
}

DetailsCategory& DetailsCustomization::AddArrayRows(UINode& InParent, DetailsTab& InTab, const WeakObjectPtr<Object>& InObject, uint64_t InOffset,
                                                    ArrayProperty* InArray, int32_t InDepth, Property* InRootProperty,
                                                    const String& InLabel, const String& InPath) {
    DetailsCategory& category = AddCategory(InParent, InTab, InLabel, InDepth + 1);
    if (IsOverridablePath(InPath)) {
        BindOverrideActions(InObject, InPath, &InTab, category.IsOverridden, category.ResetAction);
    }

    char* data = ResolveBase(InObject, InOffset);
    const int32_t count = data ? (int32_t)InArray->GetSize(data) : 0;
    const WeakObjectPtr<DetailsTab> tab = &InTab;
    category.Bind = [tab, InObject, InOffset, InArray, count] {
        char* current = ResolveBase(InObject, InOffset);
        if (current && (int32_t)InArray->GetSize(current) != count && tab.Get()) {
            tab.Get()->MarkDirty();
        }
    };

    const String name = PrettyPropertyPath(OverridablePath(InPath));
    AddContainerHeader(category, [InObject, InOffset, InArray] {
        char* current = ResolveBase(InObject, InOffset);
        return Plural(current ? InArray->GetSize(current) : 0, "element", "elements");
    }, "Add element", [=] {
        EditContainer(tab, InObject, InOffset, InRootProperty, InPath, "Add " + name + " Element", [InArray](char* InData) {
            InArray->AddDefault(InData);
        });
    }, "Remove all elements", [=] {
        EditContainer(tab, InObject, InOffset, InRootProperty, InPath, "Clear " + name, [InArray](char* InData) {
            InArray->Clear(InData);
        });
    });

    for (int32_t index = 0; index < count; index++) {
        AddArrayElement(*category.GetBody(), InTab, InObject, InOffset, InArray, index, InDepth + 1, InRootProperty, InPath);
    }
    return category;
}

void DetailsCustomization::AddArrayElement(UINode& InParent, DetailsTab& InTab, const WeakObjectPtr<Object>& InObject, uint64_t InArrayOffset,
                                           ArrayProperty* InArray, int32_t InIndex, int32_t InDepth, Property* InRootProperty,
                                           const String& InArrayPath) {
    const uint64_t offset = ArrayElementOffset(InArrayOffset, InArray, InIndex);
    String label = "Element " + std::to_string(InIndex);
    char* elementData = ResolveBase(InObject, offset);
    for (Property* member : GetInnerProperties(InArray->InnerProperty)) {
        if (elementData && member->Name == "Name" && Cast<StringProperty>(member)) {
            label = std::to_string(InIndex) + ": " + *reinterpret_cast<String*>(elementData + member->Offset);
        }
    }
    const String path = InArrayPath + "[" + std::to_string(InIndex) + "]";
    const String name = PrettyPropertyPath(OverridablePath(InArrayPath));
    const WeakObjectPtr<DetailsTab> tab = &InTab;

    const auto buildMenu = [=](UIMenuModel& OutMenu) {
        char* data = ResolveBase(InObject, InArrayOffset);
        const int32_t size = data ? (int32_t)InArray->GetSize(data) : 0;
        const auto edit = [=](const String& InTitle, std::function<void(char*)> InEdit) {
            EditContainer(tab, InObject, InArrayOffset, InRootProperty, InArrayPath, InTitle + " " + name + " Element", InEdit);
        };
        OutMenu.Section(label);
        OutMenu.Item("Insert", [=] {
            edit("Insert", [InArray, InIndex](char* InData) {
                InArray->AddDefault(InData);
                MoveArrayElement(*InArray, InData, (int32_t)InArray->GetSize(InData) - 1, InIndex);
            });
        }).Icon(EditorIcons::Add());
        OutMenu.Item("Duplicate", [=] {
            edit("Duplicate", [InArray, InIndex](char* InData) {
                InArray->AddDefault(InData);
                const int32_t last = (int32_t)InArray->GetSize(InData) - 1;
                const nlohmann::json copy = JsonSerializer::SerializeProperty(InArray->InnerProperty, InArray->GetElementPtr(InData, (size_t)InIndex));
                JsonSerializer::DeserializeProperty(InArray->InnerProperty, InArray->GetElementPtr(InData, (size_t)last), copy);
                MoveArrayElement(*InArray, InData, last, InIndex + 1);
            });
        });
        OutMenu.Separator();
        OutMenu.Item("Move Up", [=] {
            edit("Move", [InArray, InIndex](char* InData) { MoveArrayElement(*InArray, InData, InIndex, InIndex - 1); });
        }).Enabled(InIndex > 0 && InIndex < size);
        OutMenu.Item("Move Down", [=] {
            edit("Move", [InArray, InIndex](char* InData) { MoveArrayElement(*InArray, InData, InIndex, InIndex + 1); });
        }).Enabled(InIndex + 1 < size);
        OutMenu.Separator();
        OutMenu.Item("Delete", [=] {
            edit("Delete", [InArray, InIndex](char* InData) {
                if (InIndex < (int32_t)InArray->GetSize(InData)) {
                    InArray->RemoveAt(InData, (size_t)InIndex);
                }
            });
        }).Icon(EditorIcons::Delete()).Enabled(InIndex < size);
    };
    const auto addMenuButton = [buildMenu](UINode& InHost) { AddMenuButton(InHost, "Element actions", buildMenu); };

    Property* element = InArray->InnerProperty;
    const Array<Property*> members = GetInnerProperties(element);
    if (!members.IsEmpty()) {
        DetailsCategory& category = AddStructCategory(InParent, InTab, InObject, offset, members, InDepth, InRootProperty, label, path);
        addMenuButton(*category.AddHeaderValue());
        return;
    }

    DetailsRow& row = AddRow(InParent, InTab, label, InDepth);
    UINode* editor = row.GetValueHost()->Add<UINode>();
    editor->Fill();
    editor->Size = { 1.0_rel - 26.0_px, 1.0_rel };
    addMenuButton(*row.GetValueHost());

    const DetailsEditHandler onEdited = MakeEditHandler(InObject, InRootProperty, &InTab, path);
    if (BuildValueEditor(*editor, InObject, offset, element, onEdited, label)) {
        return;
    }
    bool isWeak = false;
    if (const Class* assetClass = FindAssetReferenceClass(element, isWeak)) {
        row.Height = AssetRowHeight;
        BuildAssetSlot(*editor, InTab, InObject, offset, *assetClass, isWeak, onEdited);
    } else if (StructProperty* structProperty = Cast<StructProperty>(element)) {
        AddValueLabel(*editor, structProperty->InnerStructTypename);
    }
}

DetailsCategory& DetailsCustomization::AddMapRows(UINode& InParent, DetailsTab& InTab, const WeakObjectPtr<Object>& InObject, uint64_t InOffset,
                                                  MapProperty* InMap, int32_t InDepth, Property* InRootProperty,
                                                  const String& InLabel, const String& InPath) {
    DetailsCategory& category = AddCategory(InParent, InTab, InLabel, InDepth + 1);
    if (IsOverridablePath(InPath)) {
        BindOverrideActions(InObject, InPath, &InTab, category.IsOverridden, category.ResetAction);
    }

    Array<uint64_t> entries;
    if (char* data = ResolveBase(InObject, InOffset)) {
        for (const MapProperty::Entry& entry : SortedMapEntries(*InMap, data)) {
            entries.Add(MapEntryOffset(InOffset, InMap, entry.Key));
        }
    }
    const WeakObjectPtr<DetailsTab> tab = &InTab;
    category.Bind = [tab, InObject, InOffset, InMap, entries] {
        char* current = ResolveBase(InObject, InOffset);
        if (!current || !tab.Get()) {
            return;
        }
        bool stale = (int32_t)InMap->GetSize(current) != entries.Size();
        for (int32_t i = 0; i < entries.Size() && !stale; i++) {
            stale = ResolveBase(InObject, entries[i]) == nullptr;
        }
        if (stale) {
            tab.Get()->MarkDirty();
        }
    };

    const String name = PrettyPropertyPath(OverridablePath(InPath));
    AddContainerHeader(category, [InObject, InOffset, InMap] {
        char* current = ResolveBase(InObject, InOffset);
        return Plural(current ? InMap->GetSize(current) : 0, "entry", "entries");
    }, "Add entry", [=] {
        EditContainer(tab, InObject, InOffset, InRootProperty, InPath, "Add " + name + " Entry", [InMap](char* InData) {
            InsertMapEntry(*InMap, InData, nullptr);
        });
    }, "Remove all entries", [=] {
        EditContainer(tab, InObject, InOffset, InRootProperty, InPath, "Clear " + name, [InMap](char* InData) {
            InMap->Clear(InData);
        });
    });

    for (uint64_t entry : entries) {
        AddMapEntry(*category.GetBody(), InTab, InObject, entry, InMap, InDepth + 1, InRootProperty, InPath);
    }
    return category;
}

void DetailsCustomization::AddMapEntry(UINode& InParent, DetailsTab& InTab, const WeakObjectPtr<Object>& InObject, uint64_t InEntryOffset,
                                       MapProperty* InMap, int32_t InDepth, Property* InRootProperty, const String& InMapPath) {
    const uint64_t mapOffset = GetElementAddresses()[ElementSlot(InEntryOffset)].ContainerOffset;
    const String keyLabel = MapKeyLabel(*InMap, GetElementAddresses()[ElementSlot(InEntryOffset)].KeyText);
    const String path = InMapPath + "[" + keyLabel + "]";
    const String name = PrettyPropertyPath(OverridablePath(InMapPath));
    const WeakObjectPtr<DetailsTab> tab = &InTab;
    const auto followKey = [InMap, InEntryOffset](std::function<void(const String&)> InApply) {
        return [InMap, InEntryOffset, InApply, shown = GetElementAddresses()[ElementSlot(InEntryOffset)].KeyText]() mutable {
            const String& keyText = GetElementAddresses()[ElementSlot(InEntryOffset)].KeyText;
            if (keyText != shown) {
                shown = keyText;
                InApply(MapKeyLabel(*InMap, keyText));
            }
        };
    };

    const auto buildMenu = [=](UIMenuModel& OutMenu) {
        const auto entryKey = [InEntryOffset] { return GetElementAddresses()[ElementSlot(InEntryOffset)].Key.get(); };
        OutMenu.Section(MapKeyLabel(*InMap, GetElementAddresses()[ElementSlot(InEntryOffset)].KeyText));
        OutMenu.Item("Duplicate", [=] {
            EditContainer(tab, InObject, mapOffset, InRootProperty, InMapPath, "Duplicate " + name + " Entry", [InMap, entryKey](char* InData) {
                InsertMapEntry(*InMap, InData, entryKey());
            });
        });
        OutMenu.Separator();
        OutMenu.Item("Delete", [=] {
            EditContainer(tab, InObject, mapOffset, InRootProperty, InMapPath, "Delete " + name + " Entry", [InMap, entryKey](char* InData) {
                InMap->Remove(InData, entryKey());
            });
        }).Icon(EditorIcons::Delete());
    };

    DetailsEditHandler keyEdited;
    keyEdited.CommitEdit = [tab, InObject, InEntryOffset, InRootProperty, InMapPath] {
        CommitMapKey(tab, InObject, InEntryOffset, InRootProperty, InMapPath);
    };
    const auto buildKeyEditor = [&](UINode& InHost) {
        if (!BuildValueEditor(InHost, InObject, InEntryOffset | s_MapKeyBit, InMap->KeyProperty, keyEdited, keyLabel)) {
            AddValueLabel(InHost, keyLabel);
        }
    };

    Property* value = InMap->ValueProperty;
    const Array<Property*> members = GetInnerProperties(value);
    if (!members.IsEmpty()) {
        DetailsCategory& entry = AddStructCategory(InParent, InTab, InObject, InEntryOffset, members, InDepth, InRootProperty, keyLabel, path,
                                                   [&](DetailsCategory& InCategory) {
            buildKeyEditor(*AddRow(*InCategory.GetBody(), InTab, "Key", InDepth + 1).GetValueHost());
        });
        entry.Bind = followKey([entry = &entry](const String& InLabel) { entry->SetTitle(InLabel); });
        AddMenuButton(*entry.AddHeaderValue(), "Entry actions", buildMenu);
        return;
    }

    DetailsRow& row = AddRow(InParent, InTab, keyLabel, InDepth);
    row.Bind = followKey([row = &row](const String& InLabel) { row->SetLabel(InLabel); });
    buildKeyEditor(*row.AddLabelHost());
    UINode* editor = row.GetValueHost()->Add<UINode>();
    editor->Fill();
    editor->Size = { 1.0_rel - 26.0_px, 1.0_rel };
    AddMenuButton(*row.GetValueHost(), "Entry actions", buildMenu);

    const DetailsEditHandler onEdited = MakeEditHandler(InObject, InRootProperty, &InTab, path);
    if (BuildValueEditor(*editor, InObject, InEntryOffset, value, onEdited, keyLabel)) {
        return;
    }
    bool isWeak = false;
    if (const Class* assetClass = FindAssetReferenceClass(value, isWeak)) {
        row.Height = AssetRowHeight;
        BuildAssetSlot(*editor, InTab, InObject, InEntryOffset, *assetClass, isWeak, onEdited);
    } else if (StructProperty* structProperty = Cast<StructProperty>(value)) {
        AddValueLabel(*editor, structProperty->InnerStructTypename);
    }
}

static String SplitCamelCase(const String& InName) {
    String pretty;
    for (size_t i = 0; i < InName.size(); i++) {
        const char current = InName[i];
        if (i > 0 && std::isupper((unsigned char)current) && std::islower((unsigned char)InName[i - 1])) {
            pretty += ' ';
        }
        pretty += current;
    }
    return pretty;
}

String DetailsCustomization::PrettyClassName(const Class& InClass) {
    return SplitCamelCase(InClass.GetDisplayName());
}

String DetailsCustomization::PrettyPropertyName(const String& InName) {
    return SplitCamelCase(InName.starts_with("m_") ? InName.substr(2) : InName);
}

String DetailsCustomization::PrettyPropertyPath(const String& InPath) {
    const size_t firstDot = InPath.find('.');
    if (firstDot == String::npos) {
        return PrettyPropertyName(InPath);
    }
    String pretty;
    size_t start = firstDot + 1;
    while (true) {
        const size_t dot = InPath.find('.', start);
        pretty += (pretty.empty() ? "" : " ") + PrettyPropertyName(InPath.substr(start, dot == String::npos ? String::npos : dot - start));
        if (dot == String::npos) {
            return pretty;
        }
        start = dot + 1;
    }
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

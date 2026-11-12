#include "Property.h"
#include "Common/Types.h"
#include "Serialization/Json.h"
#include "Serialization/ThirdParty/nlohmann/json.hpp"
#include <cstring>

static Map<String, Array<Property*>>& GetTypePropertyMap() {
    static Map<String, Array<Property*>> map;
    return map;
}

void Property::RegisterTypeProperties(const String& InTypename, const Array<Property*>& InProperties) {
    GetTypePropertyMap()[InTypename] += InProperties;
}

static uint64_t s_RegistryVersion = 0;

void Property::ReplaceTypeProperties(const String& InTypename, const Array<Property*>& InProperties) {
    GetTypePropertyMap()[InTypename] = InProperties;
    s_RegistryVersion++;
}

uint64_t Property::GetRegistryVersion() {
    return s_RegistryVersion;
}

Array<Property*> Property::GetTypeProperties(const String& InTypeName) {
    Map<String, Array<Property*>>& map = GetTypePropertyMap();
    if (!map.ContainsKey(InTypeName)) {
        return Array<Property*>();
    }
    return map[InTypeName];
}

Array<Property*> Property::GetAllTypeProperties(const Class& InClass) {
    Array<Class> chain;
    for (Class current = InClass; current != Class::None && current.Name != "Object"; current = current.GetParentClass()) {
        chain.Add(current);
    }

    Array<Property*> properties;
    for (int32_t i = chain.Size() - 1; i >= 0; i--) {
        properties += GetTypeProperties(chain[i].Name);
    }
    return properties;
}

Property* Property::FindTypeProperty(const Class& InClass, const String& InName) {
    for (Property* property : GetAllTypeProperties(InClass)) {
        if (property->Name == InName) {
            return property;
        }
    }
    return nullptr;
}

static_assert(sizeof(SubclassOf<Object>) == sizeof(Class));

bool Property::IsTransient() const {
    return Cast<DelegateProperty>(this) != nullptr;
}

bool SubclassOfProperty::Accepts(const Class& InClass) const {
    if (InClass == Class::None) {
        return true;
    }
    if (InClass.IsBlueprint() && InClass.GetParentClass() == Class::None) {
        return true;
    }
    return InClass.IsSubclassOf(BaseClass);
}

bool SubclassOfProperty::Write(void* InValuePtr, const Class& InClass) const {
    const bool accepted = Accepts(InClass);
    *(Class*)InValuePtr = accepted ? InClass : Class::None;
    return accepted;
}

void Property::CopyValue(void* OutInstance, const void* InInstance) const {
    const nlohmann::json value = JsonSerializer::SerializeProperty(const_cast<Property*>(this), (void*)GetValuePtr((void*)InInstance));
    JsonSerializer::DeserializeProperty(const_cast<Property*>(this), GetValuePtr(OutInstance), value);
}

void Property::CopyRawValue(void* OutValue, const void* InValue) const {
    if (const IntProperty* p = Cast<IntProperty>(this)) {
        std::memcpy(OutValue, InValue, p->NumBits / 8);
    } else if (const FloatProperty* p = Cast<FloatProperty>(this)) {
        std::memcpy(OutValue, InValue, p->IsDouble ? sizeof(double) : sizeof(float));
    } else if (Cast<BoolProperty>(this)) {
        *(bool*)OutValue = *(const bool*)InValue;
    } else if (Cast<StringProperty>(this)) {
        *(String*)OutValue = *(const String*)InValue;
    } else if (const EnumProperty* p = Cast<EnumProperty>(this)) {
        std::memcpy(OutValue, InValue, p->ByteSize);
    } else if (Cast<UUIDProperty>(this)) {
        *(UUID*)OutValue = *(const UUID*)InValue;
    } else if (Cast<SubclassOfProperty>(this)) {
        *(Class*)OutValue = *(const Class*)InValue;
    } else if (Cast<WeakObjectPtrProperty>(this)) {
        *(WeakObjectPtr<Object>*)OutValue = ((const WeakObjectPtr<Object>*)InValue)->Get();
    } else if (Cast<SharedObjectPtrProperty>(this)) {
        *(SharedObjectPtr<Object>*)OutValue = ((const SharedObjectPtr<Object>*)InValue)->Get();
    } else if (const StructProperty* p = Cast<StructProperty>(this)) {
        for (Property* member : GetTypeProperties(p->InnerStructTypename)) {
            member->CopyRawValue((char*)OutValue + member->Offset, (const char*)InValue + member->Offset);
        }
    } else if (!IsTransient()) {
        JsonSerializer::DeserializeProperty(const_cast<Property*>(this), OutValue,
                                            JsonSerializer::SerializeProperty(const_cast<Property*>(this), const_cast<void*>(InValue)));
    }
}

PropertyPath PropertyPath::Resolve(const Class& InClass, const String& InPath) {
    PropertyPath path;
    size_t start = 0;
    String typeName;
    while (true) {
        const size_t dot = InPath.find('.', start);
        const String segment = InPath.substr(start, dot == String::npos ? String::npos : dot - start);
        Property* property = path.Root ? nullptr : Property::FindTypeProperty(InClass, segment);
        if (path.Root) {
            for (Property* member : Property::GetTypeProperties(typeName)) {
                if (member->Name == segment) {
                    property = member;
                    break;
                }
            }
        }
        if (!property) {
            return PropertyPath();
        }
        if (!path.Root) {
            path.Root = property;
        }
        if (dot == String::npos) {
            path.Leaf = property;
            return path;
        }
        StructProperty* structProperty = Cast<StructProperty>(property);
        if (!structProperty) {
            return PropertyPath();
        }
        path.ContainerOffset += property->Offset;
        typeName = structProperty->InnerStructTypename;
        start = dot + 1;
    }
}

String PropertyPath::GetRootName(const String& InPath) {
    return InPath.substr(0, InPath.find('.'));
}

bool PropertyPath::IsWithin(const String& InPath, const String& InAncestor) {
    return InPath == InAncestor || (InPath.size() > InAncestor.size() && InPath.starts_with(InAncestor)
                                    && InPath[InAncestor.size()] == '.');
}

void PropertyPath::CopyValue(void* OutInstance, const void* InInstance) const {
    Leaf->CopyValue((char*)OutInstance + ContainerOffset, (const char*)InInstance + ContainerOffset);
}

struct RegisterMathProperties {
    RegisterMathProperties() {
        const auto axis = [](const char* InName, uint64_t InOffset) {
            return (Property*)new FloatProperty(InName, InOffset, false);
        };
        Property::RegisterTypeProperties("Vec2", { axis("X", 0), axis("Y", 4) });
        Property::RegisterTypeProperties("Vec3", { axis("X", 0), axis("Y", 4), axis("Z", 8) });
        Property::RegisterTypeProperties("Vec4", { axis("X", 0), axis("Y", 4), axis("Z", 8), axis("W", 12) });
        Property::RegisterTypeProperties("Quat", { axis("X", 0), axis("Y", 4), axis("Z", 8), axis("W", 12) });
        Property::RegisterTypeProperties("Color", { axis("R", 0), axis("G", 4), axis("B", 8), axis("A", 12) });
    }
};
static RegisterMathProperties s_RegisterMathProperties;

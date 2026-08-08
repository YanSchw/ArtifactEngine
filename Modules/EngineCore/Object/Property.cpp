#include "Property.h"
#include "Common/Types.h"
#include "Serialization/Json.h"
#include "Serialization/ThirdParty/nlohmann/json.hpp"

static Map<String, Array<Property*>>& GetTypePropertyMap() {
    static Map<String, Array<Property*>> map;
    return map;
}

void Property::RegisterTypeProperties(const String& InTypename, const Array<Property*>& InProperties) {
    GetTypePropertyMap()[InTypename] += InProperties;
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

void Property::CopyValue(void* OutInstance, const void* InInstance) const {
    const nlohmann::json value = JsonSerializer::SerializeProperty(const_cast<Property*>(this), (void*)GetValuePtr((void*)InInstance));
    JsonSerializer::DeserializeProperty(const_cast<Property*>(this), GetValuePtr(OutInstance), value);
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

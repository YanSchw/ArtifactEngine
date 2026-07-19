#include "Json.h"
#include "ThirdParty/nlohmann/json.hpp"

#include "Assets/AssetManager.h"
#include "Assets/Asset.h"
#include "Core/Assert.h"
#include "Object/Enum.h"

#include <cstring>

using json = nlohmann::json;

String JsonSerializer::SerializeObject(const Object* InObject) {
    json j = json::object();

    String typeName = InObject->GetClass().Name;
    j = SerializeType((void*)InObject, typeName);
    j["$type"] = typeName;

    return j.dump(4);
}
void JsonSerializer::DeserializeObject(Object* OutObject, const String& InJsonString) {
    json j = json::parse(InJsonString);

    AE_ASSERT(j.contains("$type"), "JSON does not contain type information");
    const String typeName = j["$type"].get<String>();
    AE_ASSERT(typeName == OutObject->GetClass().Name, "JSON type does not match object type");

    DeserializeType(OutObject, OutObject->GetClass().Name, j);
}

Object* JsonSerializer::DeserializeNewObject(const String& InJsonString) {
    json j = json::parse(InJsonString, nullptr, false);
    if (j.is_discarded() || !j.contains("$type") || !j["$type"].is_string()) {
        return nullptr;
    }

    Object* object = Object::Create(Class(j["$type"].get<String>()));
    if (object) {
        DeserializeType(object, object->GetClass().Name, j);
    }
    return object;
}

String JsonSerializer::SerializeStruct(const void* InStruct, const Struct& InStructType) {
    json j = json::object();
    j = SerializeType((void*)InStruct, InStructType.Name);
    return j.dump(4);
}

void JsonSerializer::DeserializeStruct(void* OutStruct, const Struct& InStructType, const String& InJsonString) {
    json j = json::parse(InJsonString);
    DeserializeType(OutStruct, InStructType.Name, j);
}

json JsonSerializer::SerializeType(void* instance, const String& typeName) {
    json j = json::object();

    auto props = Property::GetTypeProperties(typeName);

    for (Property* prop : props) {
        void* valuePtr = (char*)instance + prop->Offset;

        j[prop->Name] = SerializeProperty(prop, valuePtr);
    }

    return j;
}
void JsonSerializer::DeserializeType(void* instance, const String& typeName, const json& j) {
    auto props = Property::GetTypeProperties(typeName);

    for (Property* prop : props) {
        if (!j.contains(prop->Name))
            continue;

        void* valuePtr = (char*)instance + prop->Offset;

        DeserializeProperty(prop, valuePtr, j[prop->Name]);
    }
}

json JsonSerializer::SerializeProperty(Property* property, void* valuePtr) {
    if (auto p = Cast<IntProperty>(property)) {
        if (p->IsUnsigned) {
            switch (p->NumBits) {
                case 8:  return *(uint8_t*)valuePtr;
                case 16: return *(uint16_t*)valuePtr;
                case 32: return *(uint32_t*)valuePtr;
                case 64: return *(uint64_t*)valuePtr;
            }
        } else {
            switch (p->NumBits) {
                case 8:  return *(int8_t*)valuePtr;
                case 16: return *(int16_t*)valuePtr;
                case 32: return *(int32_t*)valuePtr;
                case 64: return *(int64_t*)valuePtr;
            }
        }
    } else if (auto p = Cast<FloatProperty>(property)) {
        if (p->IsDouble)
            return *(double*)valuePtr;
        else
            return *(float*)valuePtr;
    } else if (auto p = Cast<BoolProperty>(property)) {
        return *(bool*)valuePtr;
    } else if (auto p = Cast<StringProperty>(property)) {
        return *(String*)valuePtr;
    } else if (auto p = Cast<SharedObjectPtrProperty>(property)) {
        auto& ptr = *(SharedObjectPtr<Object>*)valuePtr;

        if (!ptr) {
            return nullptr;
        }

        return json::parse(SerializeObject(ptr.Get()));
    } else if (auto p = Cast<WeakObjectPtrProperty>(property)) {
        // Weak references don't own their target; only asset targets round-trip, by id.
        Asset* asset = Cast<Asset>((*(WeakObjectPtr<Object>*)valuePtr).Get());
        if (!asset) {
            return nullptr;
        }
        return asset->GetId().ToString();
    } else if (auto p = Cast<ArrayProperty>(property)) {
        json arr = json::array();

        size_t count = p->GetSize(valuePtr);

        for (size_t i = 0; i < count; ++i) {
            void* elem = p->GetElementPtr(valuePtr, i);
            arr.push_back(SerializeProperty(p->InnerProperty, elem));
        }

        return arr;
    } else if (auto p = Cast<EnumProperty>(property)) {
        int64_t value = 0;
        std::memcpy(&value, valuePtr, p->ByteSize);
        return Enum(p->InnerEnumTypename).ConvertValueToString(value);
    } else {
        AE_ASSERT(false, "Unsupported property type for serialization: " + property->Name);
    }
}

void JsonSerializer::DeserializeProperty(Property* property, void* valuePtr, const json& j) {
    if (auto p = Cast<IntProperty>(property)) {
        if (p->IsUnsigned) {
            switch (p->NumBits) {
                case 8:  *(uint8_t*)valuePtr = j.get<uint8_t>(); break;
                case 16: *(uint16_t*)valuePtr = j.get<uint16_t>(); break;
                case 32: *(uint32_t*)valuePtr = j.get<uint32_t>(); break;
                case 64: *(uint64_t*)valuePtr = j.get<uint64_t>(); break;
            }
        } else {
            switch (p->NumBits) {
                case 8:  *(int8_t*)valuePtr = j.get<int8_t>(); break;
                case 16: *(int16_t*)valuePtr = j.get<int16_t>(); break;
                case 32: *(int32_t*)valuePtr = j.get<int32_t>(); break;
                case 64: *(int64_t*)valuePtr = j.get<int64_t>(); break;
            }
        }

        return;
    } else if (auto p = Cast<FloatProperty>(property)) {
        if (p->IsDouble)
            *(double*)valuePtr = j.get<double>();
        else
            *(float*)valuePtr = j.get<float>();

        return;
    } else if (auto p = Cast<BoolProperty>(property)) {
        *(bool*)valuePtr = j.get<bool>();
        return;
    } else if (auto p = Cast<StringProperty>(property)) {
        *(String*)valuePtr = j.get<String>();
        return;
    } else if (auto p = Cast<SharedObjectPtrProperty>(property)) {
        if (j.is_null()) {
            *(SharedObjectPtr<Object>*)valuePtr = nullptr;
            return;
        }

        // Instantiate the concrete runtime type recorded in $type, so
        // polymorphic pointers (e.g. InputBinding -> PathBinding) round-trip
        // instead of always creating the declared inner class.
        AE_ASSERT(j.contains("$type"), "Serialized object pointer is missing $type");
        Object* obj = Object::Create(Class(j["$type"].get<String>()));
        DeserializeObject(obj, j.dump());
        *(SharedObjectPtr<Object>*)valuePtr = SharedObjectPtr<Object>(obj);

        return;
    } else if (auto p = Cast<WeakObjectPtrProperty>(property)) {
        *(WeakObjectPtr<Object>*)valuePtr = j.is_null()
            ? nullptr
            : (Object*)AssetManager::Get().GetAsset(UUID::FromString(j.get<String>()));
        return;
    } else if (auto p = Cast<ArrayProperty>(property)) {
        for (auto& elemJson : j) {
            p->AddDefault(valuePtr);

            size_t last = p->GetSize(valuePtr) - 1;
            void* elemPtr = p->GetElementPtr(valuePtr, last);

            DeserializeProperty(p->InnerProperty, elemPtr, elemJson);
        }
    } else if (auto p = Cast<EnumProperty>(property)) {
        int64_t value = Enum(p->InnerEnumTypename).ConvertStringToValue(j.get<String>());
        std::memcpy(valuePtr, &value, p->ByteSize);
    } else {
        AE_ASSERT(false, "Unsupported property type for deserialization: " + property->Name);
    }
}
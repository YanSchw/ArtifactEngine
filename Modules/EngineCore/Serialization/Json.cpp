#include "Json.h"
#include "ThirdParty/nlohmann/json.hpp"

#include "Core/Assert.h"

using json = nlohmann::json;

String JsonSerializer::SerializeObject(const Object* InObject) {
    json j = json::object();

    String typeName = InObject->GetClass().Name;
    j = SerializeType((void*)InObject, typeName);
    j["$type"] = typeName;

    return j.dump();
}
void JsonSerializer::DeserializeObject(Object* OutObject, const String& InJsonString) {
    json j = json::parse(InJsonString);

    AE_ASSERT(j.contains("$type"), "JSON does not contain type information");
    const String typeName = j["$type"].get<String>();
    AE_ASSERT(typeName == OutObject->GetClass().Name, "JSON type does not match object type");

    DeserializeType(OutObject, OutObject->GetClass().Name, j);
}

String JsonSerializer::SerializeStruct(const void* InStruct, const Struct& InStructType) {
    json j = json::object();
    j = SerializeType((void*)InStruct, InStructType.Name);
    return j.dump();
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
    } else if (auto p = Cast<SharedObjectPtrProperty>(property)) {
        auto& ptr = *(SharedObjectPtr<Object>*)valuePtr;

        if (!ptr) {
            return nullptr;
        }

        return json::parse(SerializeObject(ptr.Get()));
    } else if (auto p = Cast<ArrayProperty>(property)) {
        json arr = json::array();

        size_t count = p->GetSize(valuePtr);

        for (size_t i = 0; i < count; ++i) {
            void* elem = p->GetElementPtr(valuePtr, i);
            arr.push_back(SerializeProperty(p->InnerProperty, elem));
        }

        return arr;
    } else {
        AE_ASSERT(false, "Unsupported property type for serialization: " + property->Name);
    }
}

void JsonSerializer::DeserializeProperty(Property* property, void* valuePtr, const json& j) {
    if (auto p = Cast<IntProperty>(property)) {
        if (p->IsUnsigned) {
            switch (p->NumBits) {
                case 8:  *(uint8_t*)valuePtr = j.get<uint8_t>();
                case 16: *(uint16_t*)valuePtr = j.get<uint16_t>();
                case 32: *(uint32_t*)valuePtr = j.get<uint32_t>();
                case 64: *(uint64_t*)valuePtr = j.get<uint64_t>();
            }
        } else {
            switch (p->NumBits) {
                case 8:  *(int8_t*)valuePtr = j.get<int8_t>();
                case 16: *(int16_t*)valuePtr = j.get<int16_t>();
                case 32: *(int32_t*)valuePtr = j.get<int32_t>();
                case 64: *(int64_t*)valuePtr = j.get<int64_t>();
            }
        }

        return;
    } else if (auto p = Cast<FloatProperty>(property)) {
        if (p->IsDouble)
            *(double*)valuePtr = j.get<double>();
        else
            *(float*)valuePtr = j.get<float>();

        return;
    } else if (auto p = Cast<SharedObjectPtrProperty>(property)) {
        if (j.is_null()) {
            *(SharedObjectPtr<Object>*)valuePtr = nullptr;
            return;
        }

        Object* obj = Object::Create(p->InnerClass);
        DeserializeObject(obj, j.dump());
        *(SharedObjectPtr<Object>*)valuePtr = SharedObjectPtr<Object>(obj);

        return;
    } else if (auto p = Cast<ArrayProperty>(property)) {
        for (auto& elemJson : j) {
            p->AddDefault(valuePtr);

            size_t last = p->GetSize(valuePtr) - 1;
            void* elemPtr = p->GetElementPtr(valuePtr, last);

            DeserializeProperty(p->InnerProperty, elemPtr, elemJson);
        }
    } else {
        AE_ASSERT(false, "Unsupported property type for deserialization: " + property->Name);
    }
}
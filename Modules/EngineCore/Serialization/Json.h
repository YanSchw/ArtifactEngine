#pragma once
#include "CoreMinimal.h"
#include "ThirdParty/nlohmann/json_fwd.hpp"

class JsonSerializer {
    using json = nlohmann::json;
public:
    static String SerializeObject(const Object* InObject);
    static void DeserializeObject(Object* OutObject, const String& InJsonString);

    static String SerializeStruct(const void* InStruct, const Struct& InStructType);
    static void DeserializeStruct(void* OutStruct, const Struct& InStructType, const String& InJsonString);

private:
    static json SerializeType(void* instance, const String& typeName);
    static void DeserializeType(void* instance, const String& typeName, const json& j);

    static json SerializeProperty(Property* property, void* valuePtr);
    static void DeserializeProperty(Property* property, void* valuePtr, const json& j);
};
#pragma once
#include "Common/String.h"
#include "Common/Array.h"
#include "Common/Map.h"

struct Enum {
    String Name;
    
    Enum(const String& name) : Name(name) {}

    int64_t ConvertStringToValue(const String& InString) const;
    String ConvertValueToString(int64_t InValue) const;

    struct EnumValue {
        String Name;
        int64_t Value;
    };
private:
    inline static Map<String, Array<EnumValue>> s_EnumValues;
public:
    struct RegisterEnumValues {
        RegisterEnumValues(const String& InEnumName, const Array<EnumValue>& InValues) {
            s_EnumValues[InEnumName] = InValues;
        }
    };
};

#ifndef GENERATED_BODY /* in-case Object.h is included */
#define GENERATED_BODY_CONCAT(line) _GENERATED_BODY_##line
#define GENERATED_BODY(line) GENERATED_BODY_CONCAT(line)
#endif

#define ARTIFACT_ENUM(...) GENERATED_BODY(__LINE__);

#pragma once
#include "Core/Log.h"
#include "Common/String.h"
#include "Enum.h"

int64_t Enum::ConvertStringToValue(const String& InString) const {
    if (Enum::s_EnumValues.ContainsKey(Name)) {
        const auto& values = Enum::s_EnumValues[Name];
        for (const auto& value : values) {
            if (value.Name == InString) {
                return value.Value;
            }
        }
        AE_ERROR("Enum {0} does not contain a value named {1}", Name, InString);
    }
    else {
        AE_ERROR("Enum {0} does not have any registered values", Name);
    }
    return -1;
}

String Enum::ConvertValueToString(int64_t InValue) const {
    if (Enum::s_EnumValues.ContainsKey(Name)) {
        const auto& values = Enum::s_EnumValues[Name];
        for (const auto& value : values) {
            if (value.Value == InValue) {
                return value.Name;
            }
        }
        AE_ERROR("Enum {0} does not contain a value with ID {1}", Name, InValue);
    }
    else {
        AE_ERROR("Enum {0} does not have any registered values", Name);
    }
    return String();
}
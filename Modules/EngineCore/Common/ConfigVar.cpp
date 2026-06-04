#include "ConfigVar.h"

bool ConfigVar::Update(const String& InNewValue) {
    if (Frozen) {
        return false;
    }
    Value = InNewValue;
    return true;
}

bool ConfigVar::IsFrozen() const {
    return Frozen;
}

void ConfigVar::Freeze() {
    Frozen = true;
}

template<>
String ConfigVar::As<String>(String InDefault) const {
    return Value;
}

template<>
int ConfigVar::As<int>(int InDefault) const {
    try {
        return std::stoi(Value);
    } catch (...) {
        return InDefault;
    }
}

template<>
bool ConfigVar::As<bool>(bool InDefault) const {
    if (Value == "true" || Value == "1") {
        return true;
    }
    if (Value == "false" || Value == "0") {
        return false;
    }
    return InDefault;
}
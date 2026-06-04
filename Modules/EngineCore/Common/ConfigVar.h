#pragma once
#include "String.h"

struct ConfigVar {
    ConfigVar() = default;
    ConfigVar(const String& InValue) : Value(InValue) {}

    template<typename T>
    T As(T InDefault) const;

    bool Update(const String& InNewValue);
    bool IsFrozen() const;
    void Freeze();
private:
    String Value;
    bool Frozen = false;
};
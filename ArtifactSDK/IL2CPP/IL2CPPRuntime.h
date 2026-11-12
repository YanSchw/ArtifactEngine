#pragma once
#include "Object/Object.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <string>
#include <type_traits>

namespace IL2CPP {

template<typename T>
T* New() {
    T* object = Object::Create<T>();
    Object::Retain(object);
    return object;
}

inline String ToString(const String& InValue) {
    return InValue;
}

inline String ToString(bool InValue) {
    return InValue ? "True" : "False";
}

inline String ToString(double InValue) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%g", InValue);
    return buffer;
}

template<typename T> requires std::is_integral_v<T>
String ToString(T InValue) {
    return std::to_string(InValue);
}

}

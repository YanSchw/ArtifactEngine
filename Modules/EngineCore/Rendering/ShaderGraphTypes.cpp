#include "ShaderGraphTypes.h"

#include <format>

String ShaderValue::GetGlslType(ShaderValueType InType) {
    switch (InType) {
        case ShaderValueType::Float: return "float";
        case ShaderValueType::Vec2:  return "vec2";
        case ShaderValueType::Vec3:  return "vec3";
        case ShaderValueType::Vec4:  return "vec4";
        case ShaderValueType::Color: return "vec4";
        case ShaderValueType::Texture2D: return "sampler2D";
    }
    return "float";
}

bool ShaderValue::ParseGlslType(const String& InText, ShaderValueType& OutType) {
    if (InText == "float") { OutType = ShaderValueType::Float; return true; }
    if (InText == "vec2")  { OutType = ShaderValueType::Vec2; return true; }
    if (InText == "vec3")  { OutType = ShaderValueType::Vec3; return true; }
    if (InText == "vec4")  { OutType = ShaderValueType::Vec4; return true; }
    if (InText == "Color") { OutType = ShaderValueType::Color; return true; }
    if (InText == "sampler2D" || InText == "Texture2D") { OutType = ShaderValueType::Texture2D; return true; }
    return false;
}

uint32_t ShaderValue::GetComponentCount(ShaderValueType InType) {
    switch (InType) {
        case ShaderValueType::Float: return 1;
        case ShaderValueType::Vec2:  return 2;
        case ShaderValueType::Vec3:  return 3;
        case ShaderValueType::Vec4:  return 4;
        case ShaderValueType::Color: return 4;
        case ShaderValueType::Texture2D: return 0;
    }
    return 1;
}

uint32_t ShaderValue::GetAlignment(ShaderValueType InType) {
    const uint32_t components = GetComponentCount(InType);
    return components == 1 ? 4 : (components == 2 ? 8 : 16);
}

uint32_t ShaderValue::GetSize(ShaderValueType InType) {
    return GetComponentCount(InType) * 4;
}

String ShaderValue::Convert(const String& InExpression, ShaderValueType InFrom, ShaderValueType InTo) {
    if (InFrom == InTo || InFrom == ShaderValueType::Texture2D || InTo == ShaderValueType::Texture2D) {
        return InExpression;
    }

    const uint32_t from = GetComponentCount(InFrom);
    const uint32_t to = GetComponentCount(InTo);

    if (from == 1) {
        return std::format("{0}({1})", GetGlslType(InTo), InExpression);
    }
    if (from > to) {
        static const char* s_Swizzle[] = { "", "x", "xy", "xyz", "xyzw" };
        return std::format("({0}).{1}", InExpression, s_Swizzle[to]);
    }

    String padding;
    for (uint32_t i = from; i < to; i++) {
        padding += i == 3 ? ", 1.0" : ", 0.0";
    }
    return std::format("{0}({1}{2})", GetGlslType(InTo), InExpression, padding);
}

String ShaderValue::Literal(const Vec4& InValue, ShaderValueType InType) {
    switch (InType) {
        case ShaderValueType::Float: return std::format("{:.6}", InValue.x);
        case ShaderValueType::Vec2:  return std::format("vec2({:.6}, {:.6})", InValue.x, InValue.y);
        case ShaderValueType::Vec3:  return std::format("vec3({:.6}, {:.6}, {:.6})", InValue.x, InValue.y, InValue.z);
        case ShaderValueType::Vec4:
        case ShaderValueType::Color: return std::format("vec4({:.6}, {:.6}, {:.6}, {:.6})", InValue.x, InValue.y, InValue.z, InValue.w);
        default: break;
    }
    return "0.0";
}

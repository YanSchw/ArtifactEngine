#pragma once
#include "CoreMinimal.h"
#include "ShaderGraphTypes.gen.h"

ARTIFACT_ENUM();
enum class ShaderValueType : uint32_t {
    Float,
    Vec2,
    Vec3,
    Vec4,
    Texture2D,
    Color
};

struct ShaderGraphProperty {
    ARTIFACT_STRUCT();

    PROPERTY()
    String Name;

    PROPERTY()
    String Identifier;

    PROPERTY()
    ShaderValueType Type = ShaderValueType::Float;

    PROPERTY()
    Vec4 DefaultValue = Vec4(0.0f);

    bool IsTexture() const { return Type == ShaderValueType::Texture2D; }
    bool IsColor() const { return Type == ShaderValueType::Color; }
};

class ShaderValue {
public:
    static String GetGlslType(ShaderValueType InType);
    static bool ParseGlslType(const String& InText, ShaderValueType& OutType);
    static uint32_t GetComponentCount(ShaderValueType InType);
    static uint32_t GetAlignment(ShaderValueType InType);
    static uint32_t GetSize(ShaderValueType InType);

    static String Convert(const String& InExpression, ShaderValueType InFrom, ShaderValueType InTo);
    static String Literal(const Vec4& InValue, ShaderValueType InType);
};

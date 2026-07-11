#pragma once
#include "Common/Types.h"
#include "Rendering/ShaderDataType.h"

struct UIVertex {
    Vec3 Position;
    Vec4 Color;
    Vec2 TexCoord;

    static Array<ShaderDataType> GetLayout() {
        return { ShaderDataType::Float3, ShaderDataType::Float4, ShaderDataType::Float2 };
    }
};
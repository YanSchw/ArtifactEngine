#pragma once
#include "Common/Types.h"
#include "ShaderDataType.h"

struct Vertex {
    Vec3 Position;
    Vec3 Color;
    Vec2 TexCoord;
    Vec3 Normal;

    static Array<ShaderDataType> GetLayout() {
        return { ShaderDataType::Float3, ShaderDataType::Float3, ShaderDataType::Float2, ShaderDataType::Float3 };
    }
};

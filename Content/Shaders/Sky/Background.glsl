#pragma once


#include "/Shaders/Sky/Common.glsl"

layout(location = 0) in vec4 v_Near;
layout(location = 1) in vec4 v_Far;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNodeId;

vec3 ViewDirection() {
    return normalize(v_Far.xyz / v_Far.w - v_Near.xyz / v_Near.w);
}

void WriteSky(vec3 InColor) {
    outColor = vec4(InColor * u_SkyParams.x, 1.0);
    outNodeId = vec4(0.0);
}

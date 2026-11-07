// Mirrors SceneUniformData. Included once per stage, so no #pragma once.

layout(binding = 0, std140) uniform SceneBlock {
    mat4 u_ViewProjection;
    mat4 u_InverseViewProjection;
    mat4 u_ShadowMatrices[4];
    vec4 u_CameraPosition;
    vec4 u_SunDirection;
    vec4 u_SunColor;
    vec4 u_AmbientColor;
    vec4 u_CascadeTexelSizes;
    vec4 u_ShadowParams;
    vec4 u_PointLightPositions[16];
    vec4 u_PointLightColors[16];
    float u_PointLightCount;
    float u_Time;
};

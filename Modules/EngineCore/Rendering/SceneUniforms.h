#pragma once
#include "Common/Types.h"

/** Mirrors the std140 SceneBlock that ShaderTemplate emits at every #gen_buffers site.
 *  Whoever renders a scene fills one of these and uploads it to binding 0. */
struct SceneUniformData {
    static constexpr int32_t ShadowCascadeCount = 4;
    static constexpr int32_t MaxPointLights = 16;

    Mat4 ViewProjection = Mat4(1.0f);
    Mat4 ShadowMatrices[ShadowCascadeCount] = { Mat4(1.0f), Mat4(1.0f), Mat4(1.0f), Mat4(1.0f) };

    /** xyz: the direction the sun light travels in. */
    Vec4 SunDirection = Vec4(0.0f, -1.0f, 0.0f, 0.0f);
    /** rgb: color premultiplied by intensity. */
    Vec4 SunColor = Vec4(0.0f);
    Vec4 AmbientColor = Vec4(0.0f);

    /** World-space size of one shadow texel, per cascade. */
    Vec4 CascadeTexelSizes = Vec4(0.0f);
    /** x: depth bias, y: normal bias in texels, z: active cascade count, w: shadow map resolution. */
    Vec4 ShadowParams = Vec4(0.0f);

    /** xyz: world position, w: attenuation radius. */
    Vec4 PointLightPositions[MaxPointLights] = {};
    /** rgb: color premultiplied by intensity. */
    Vec4 PointLightColors[MaxPointLights] = {};

    float PointLightCount = 0.0f;
    float Time = 0.0f;
    float Padding[2] = { 0.0f, 0.0f };
};

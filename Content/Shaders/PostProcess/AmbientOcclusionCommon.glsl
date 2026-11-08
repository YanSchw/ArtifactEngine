#pragma once

// Mirrors AmbientOcclusionUniforms in AmbientOcclusionPass.cpp.
layout(binding = 1, std140) uniform AmbientOcclusionBlock {
    mat4 u_InverseProjection;
    // x: radius, y: bias, z: intensity, w: power
    vec4 u_AoShape;
    // x: pixels one world unit spans at depth 1 (at any depth when orthographic), y: 1 when orthographic,
    // z: sample count, w: noise offset of this frame
    vec4 u_AoSampling;
    // x: fade start distance, y: max distance, z: blur sharpness, w: luminance influence
    vec4 u_AoFade;
    vec4 u_AoTint;
    // xy: size of the occlusion target, zw: size of the scene
    vec4 u_AoResolution;
};

layout(binding = 16) uniform sampler2D u_Depth;

const float FarViewDepth = 60000.0;

vec3 ViewPosition(vec2 InUV, float InDepth) {
    vec4 position = u_InverseProjection * vec4(InUV * 2.0 - 1.0, InDepth, 1.0);
    return position.xyz / position.w;
}

vec3 ViewPositionAt(ivec2 InTexel) {
    ivec2 size = textureSize(u_Depth, 0);
    ivec2 texel = clamp(InTexel, ivec2(0), size - 1);
    return ViewPosition((vec2(texel) + 0.5) / vec2(size), texelFetch(u_Depth, texel, 0).r);
}

float ViewDepthAt(ivec2 InTexel) {
    ivec2 size = textureSize(u_Depth, 0);
    ivec2 texel = clamp(InTexel, ivec2(0), size - 1);
    float depth = texelFetch(u_Depth, texel, 0).r;
    if (depth >= 1.0) {
        return FarViewDepth;
    }
    return min(ViewPosition((vec2(texel) + 0.5) / vec2(size), depth).z, FarViewDepth);
}
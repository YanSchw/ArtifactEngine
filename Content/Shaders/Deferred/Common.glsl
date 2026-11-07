#pragma once

// Mirrors GBufferLayout. Shared by every fullscreen pass that reads the GBuffer back.

#include "/Shaders/Common/Scene.glsl"
#include "/Shaders/Common/Encoding.glsl"

layout(binding = 2) uniform sampler2DArrayShadow u_ShadowMap;
layout(binding = 3) uniform sampler2D u_GBufferAlbedo;
layout(binding = 4) uniform sampler2D u_GBufferNormal;
layout(binding = 5) uniform sampler2D u_GBufferEmissive;
layout(binding = 6) uniform sampler2D u_GBufferNodeId;
layout(binding = 7) uniform sampler2D u_GBufferDepth;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNodeId;

struct GBufferTexel {
    vec3 Albedo;
    vec3 Normal;
    vec3 Emissive;
    vec4 NodeId;
    vec3 WorldPosition;
    float Depth;
};

/** False where no deferred geometry was drawn. */
bool ReadGBuffer(out GBufferTexel OutTexel) {
    ivec2 texel = ivec2(gl_FragCoord.xy);
    OutTexel.Depth = texelFetch(u_GBufferDepth, texel, 0).r;
    if (OutTexel.Depth >= 1.0) {
        return false;
    }

    OutTexel.Albedo = texelFetch(u_GBufferAlbedo, texel, 0).rgb;
    OutTexel.Normal = DecodeNormal(texelFetch(u_GBufferNormal, texel, 0).xyz);
    OutTexel.Emissive = texelFetch(u_GBufferEmissive, texel, 0).rgb;
    OutTexel.NodeId = texelFetch(u_GBufferNodeId, texel, 0);

    vec2 ndc = gl_FragCoord.xy / vec2(textureSize(u_GBufferDepth, 0)) * 2.0 - 1.0;
    vec4 world = u_InverseViewProjection * vec4(ndc, OutTexel.Depth, 1.0);
    OutTexel.WorldPosition = world.xyz / world.w;
    return true;
}

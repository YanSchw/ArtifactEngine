#blend Opaque
#depth None
#cull None

#type vert
layout(location = 0) in vec3 a_Position;

void main() {
    gl_Position = vec4(a_Position, 1.0);
}

#type frag
#include "/Shaders/PostProcess/AmbientOcclusionCommon.glsl"

layout(binding = 17) uniform sampler2D u_Occlusion;

layout(location = 0) out vec4 outOcclusion;

void main() {
    ivec2 size = textureSize(u_Occlusion, 0);
    ivec2 texel = ivec2(gl_FragCoord.xy);
    vec2 center = texelFetch(u_Occlusion, texel, 0).rg;
    if (center.g >= FarViewDepth) {
        outOcclusion = vec4(center, 0.0, 1.0);
        return;
    }

    float sum = 0.0;
    float weightSum = 0.0;
    for (int y = -2; y <= 2; y++) {
        for (int x = -2; x <= 2; x++) {
            vec2 tap = texelFetch(u_Occlusion, clamp(texel + ivec2(x, y), ivec2(0), size - 1), 0).rg;
            float spatial = exp(-0.18 * float(x * x + y * y));
            float relativeDepth = abs(tap.g - center.g) / max(center.g, 1e-4);
            float weight = spatial * exp(-relativeDepth * u_AoFade.z * 10.0);
            sum += tap.r * weight;
            weightSum += weight;
        }
    }
    outOcclusion = vec4(sum / weightSum, center.g, 0.0, 1.0);
}

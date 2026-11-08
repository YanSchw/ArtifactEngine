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
layout(binding = 18) uniform sampler2D u_Source;

layout(location = 0) out vec4 outColor;

// Bilinear from the occlusion target, but only across texels that lie at this pixel's depth.
float UpsampleOcclusion(ivec2 InTexel) {
    ivec2 size = textureSize(u_Occlusion, 0);
    if (size == textureSize(u_Depth, 0)) {
        return texelFetch(u_Occlusion, InTexel, 0).r;
    }
    float depth = ViewDepthAt(InTexel);
    vec2 position = (vec2(InTexel) + 0.5) * vec2(size) / u_AoResolution.zw - 0.5;
    ivec2 base = ivec2(floor(position));
    vec2 f = position - vec2(base);

    float sum = 0.0;
    float weightSum = 0.0;
    for (int y = 0; y <= 1; y++) {
        for (int x = 0; x <= 1; x++) {
            vec2 tap = texelFetch(u_Occlusion, clamp(base + ivec2(x, y), ivec2(0), size - 1), 0).rg;
            float bilinear = (x == 0 ? 1.0 - f.x : f.x) * (y == 0 ? 1.0 - f.y : f.y);
            float relativeDepth = abs(tap.g - depth) / max(depth, 1e-4);
            float weight = (bilinear + 1e-3) / (1e-3 + relativeDepth * u_AoFade.z * 10.0);
            sum += tap.r * weight;
            weightSum += weight;
        }
    }
    return sum / weightSum;
}

void main() {
    ivec2 texel = ivec2(gl_FragCoord.xy);
    vec3 color = texelFetch(u_Source, texel, 0).rgb;
    float visibility = UpsampleOcclusion(texel);

    float luma = dot(clamp(color, 0.0, 1.0), vec3(0.2126, 0.7152, 0.0722));
    visibility = mix(visibility, 1.0, luma * u_AoFade.w);
    outColor = vec4(color * mix(u_AoTint.rgb, vec3(1.0), visibility), 1.0);
}

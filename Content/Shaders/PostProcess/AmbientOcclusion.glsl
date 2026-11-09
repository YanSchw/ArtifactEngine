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

layout(location = 0) out vec4 outOcclusion;

const float TwoPi = 6.28318530718;
const float SpiralTurns = 7.0;

float InterleavedGradientNoise(vec2 InPixel) {
    return fract(52.9829189 * fract(dot(InPixel, vec2(0.06711056, 0.00583715))));
}

// Picks the neighbour on the same surface on each axis, so normals stay true along depth edges.
vec3 ReconstructNormal(ivec2 InTexel, vec3 InCenter, bool InOrthographic) {
    vec3 left = ViewPositionAt(InTexel - ivec2(1, 0));
    vec3 right = ViewPositionAt(InTexel + ivec2(1, 0));
    vec3 down = ViewPositionAt(InTexel - ivec2(0, 1));
    vec3 up = ViewPositionAt(InTexel + ivec2(0, 1));
    vec3 dx = abs(right.z - InCenter.z) < abs(InCenter.z - left.z) ? right - InCenter : InCenter - left;
    vec3 dy = abs(up.z - InCenter.z) < abs(InCenter.z - down.z) ? up - InCenter : InCenter - down;
    vec3 normal = normalize(cross(dx, dy));
    vec3 towardsCamera = InOrthographic ? vec3(0.0, 0.0, -1.0) : -InCenter;
    return dot(normal, towardsCamera) < 0.0 ? -normal : normal;
}

void main() {
    vec2 uv = gl_FragCoord.xy / u_AoResolution.xy;
    ivec2 depthSize = textureSize(u_Depth, 0);
    ivec2 texel = clamp(ivec2(uv * vec2(depthSize)), ivec2(0), depthSize - 1);
    float depth = texelFetch(u_Depth, texel, 0).r;
    if (depth >= 1.0) {
        outOcclusion = vec4(1.0, FarViewDepth, 0.0, 1.0);
        return;
    }

    bool orthographic = u_AoSampling.y > 0.5;
    vec3 center = ViewPosition((vec2(texel) + 0.5) / vec2(depthSize), depth);
    vec3 normal = ReconstructNormal(texel, center, orthographic);

    float radius = max(u_AoShape.x, 1e-4);
    float radiusPixels = u_AoSampling.x * radius / (orthographic ? 1.0 : max(center.z, 1e-4));
    radiusPixels = min(radiusPixels, 0.25 * float(depthSize.y));
    float fade = 1.0 - smoothstep(u_AoFade.x, u_AoFade.y, center.z);
    if (radiusPixels < 1.0 || fade <= 0.0) {
        outOcclusion = vec4(1.0, center.z, 0.0, 1.0);
        return;
    }

    int sampleCount = int(u_AoSampling.z);
    float rotation = InterleavedGradientNoise(gl_FragCoord.xy + u_AoSampling.w) * TwoPi;
    float jitter = InterleavedGradientNoise(gl_FragCoord.yx + 17.0 + u_AoSampling.w);
    float radiusSquared = radius * radius;
    float occlusion = 0.0;
    for (int i = 0; i < sampleCount; i++) {
        float alpha = (float(i) + jitter) / float(sampleCount);
        float angle = alpha * SpiralTurns * TwoPi + rotation;
        vec2 offset = vec2(cos(angle), sin(angle)) * max(alpha * radiusPixels, 1.0);
        vec3 occluder = ViewPositionAt(texel + ivec2(offset));
        vec3 toOccluder = occluder - center;
        float distanceSquared = dot(toOccluder, toOccluder);
        float falloff = max(1.0 - distanceSquared / radiusSquared, 0.0);
        float cosine = (dot(toOccluder, normal) - u_AoShape.y) / (sqrt(distanceSquared) + 1e-4);
        occlusion += max(cosine, 0.0) * falloff;
    }
    occlusion /= float(sampleCount);

    float visibility = pow(clamp(1.0 - occlusion * u_AoShape.z, 0.0, 1.0), u_AoShape.w);
    outOcclusion = vec4(mix(1.0, visibility, fade), center.z, 0.0, 1.0);
}

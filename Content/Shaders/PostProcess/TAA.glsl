#blend Opaque
#depth None
#cull None

#type vert
layout(location = 0) in vec3 a_Position;

void main() {
    gl_Position = vec4(a_Position, 1.0);
}

#type frag
// Mirrors TaaUniforms in AntiAliasingPass.cpp. Both matrices are unjittered.
layout(binding = 1, std140) uniform TaaBlock {
    mat4 u_InverseViewProjection;
    mat4 u_PreviousViewProjection;
    // x: history feedback, y: 1 while the history is valid, zw: this frame's jitter in uv units
    vec4 u_TaaParams;
};
layout(binding = 16) uniform sampler2D u_Current;
layout(binding = 17) uniform sampler2D u_History;
layout(binding = 18) uniform sampler2D u_Depth;

layout(location = 0) out vec4 outColor;

vec3 ToYCoCg(vec3 InColor) {
    return vec3(0.25 * InColor.r + 0.5 * InColor.g + 0.25 * InColor.b,
                0.5 * InColor.r - 0.5 * InColor.b,
                -0.25 * InColor.r + 0.5 * InColor.g - 0.25 * InColor.b);
}

vec3 FromYCoCg(vec3 InColor) {
    return vec3(InColor.x + InColor.y - InColor.z,
                InColor.x + InColor.z,
                InColor.x - InColor.y - InColor.z);
}

// Bicubic Catmull-Rom from five bilinear taps, which keeps the history sharp while it is resampled.
vec3 SampleHistory(vec2 InUV, vec2 InSize) {
    vec2 position = InUV * InSize;
    vec2 center = floor(position - 0.5) + 0.5;
    vec2 f = position - center;

    vec2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    vec2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    vec2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
    vec2 w3 = f * f * (-0.5 + 0.5 * f);
    vec2 w12 = w1 + w2;

    vec2 uv0 = (center - 1.0) / InSize;
    vec2 uv3 = (center + 2.0) / InSize;
    vec2 uv12 = (center + w2 / w12) / InSize;

    vec3 result = texture(u_History, vec2(uv12.x, uv0.y)).rgb * w12.x * w0.y
                + texture(u_History, vec2(uv0.x, uv12.y)).rgb * w0.x * w12.y
                + texture(u_History, uv12).rgb * w12.x * w12.y
                + texture(u_History, vec2(uv3.x, uv12.y)).rgb * w3.x * w12.y
                + texture(u_History, vec2(uv12.x, uv3.y)).rgb * w12.x * w3.y;
    float weight = w12.x * w0.y + w0.x * w12.y + w12.x * w12.y + w3.x * w12.y + w12.x * w3.y;
    return max(result / weight, vec3(0.0));
}

void main() {
    ivec2 size = textureSize(u_Current, 0);
    ivec2 texel = ivec2(gl_FragCoord.xy);
    vec2 uv = gl_FragCoord.xy / vec2(size);

    // The scene was rendered jittered, so the point at this pixel's centre sits off by the jitter.
    vec3 current = ToYCoCg(clamp(texture(u_Current, uv + u_TaaParams.zw).rgb, 0.0, 1.0));
    if (u_TaaParams.y < 0.5) {
        outColor = vec4(FromYCoCg(current), 1.0);
        return;
    }

    vec3 neighbourhoodMin = current;
    vec3 neighbourhoodMax = current;
    float closestDepth = 1.0;
    ivec2 closestTexel = texel;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            ivec2 neighbour = clamp(texel + ivec2(x, y), ivec2(0), size - 1);
            vec3 color = ToYCoCg(clamp(texelFetch(u_Current, neighbour, 0).rgb, 0.0, 1.0));
            neighbourhoodMin = min(neighbourhoodMin, color);
            neighbourhoodMax = max(neighbourhoodMax, color);
            float depth = texelFetch(u_Depth, neighbour, 0).r;
            if (depth < closestDepth) {
                closestDepth = depth;
                closestTexel = neighbour;
            }
        }
    }

    // Camera-only reprojection, taken from the nearest surface around so edges carry their own motion.
    vec2 closestUV = (vec2(closestTexel) + 0.5) / vec2(size);
    vec4 world = u_InverseViewProjection * vec4(closestUV * 2.0 - 1.0, closestDepth, 1.0);
    vec4 previousClip = u_PreviousViewProjection * vec4(world.xyz / world.w, 1.0);
    vec2 historyUV = uv + (previousClip.xy / previousClip.w * 0.5 + 0.5) - closestUV;

    float feedback = u_TaaParams.x;
    if (any(lessThan(historyUV, vec2(0.0))) || any(greaterThan(historyUV, vec2(1.0)))) {
        feedback = 0.0;
    }

    // Pull the history into the colours present around this pixel now, which rejects most ghosts.
    vec3 history = ToYCoCg(SampleHistory(historyUV, vec2(size)));
    vec3 center = 0.5 * (neighbourhoodMax + neighbourhoodMin);
    vec3 extents = 0.5 * (neighbourhoodMax - neighbourhoodMin) + 1e-4;
    vec3 offset = history - center;
    vec3 scale = abs(offset / extents);
    float largest = max(scale.x, max(scale.y, scale.z));
    if (largest > 1.0) {
        history = center + offset / largest;
    }

    outColor = vec4(FromYCoCg(mix(current, history, feedback)), 1.0);
}

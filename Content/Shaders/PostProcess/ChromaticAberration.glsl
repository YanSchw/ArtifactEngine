#blend Opaque
#depth None
#cull None

#type vert
layout(location = 0) in vec3 a_Position;

void main() {
    gl_Position = vec4(a_Position, 1.0);
}

#type frag
// Mirrors ChromaticAberrationUniforms in ChromaticAberrationPass.cpp.
layout(binding = 1, std140) uniform ChromaticAberrationBlock {
    // x: split between the outermost colours at the corners, as a share of the screen's height,
    // y: start radius, z: falloff, w: sample count
    vec4 u_CaParams;
    // x: aspect ratio
    vec4 u_CaScreen;
};
layout(binding = 16) uniform sampler2D u_Source;

layout(location = 0) out vec4 outColor;

// Red at 0, green at 0.5, blue at 1.
vec3 SpectrumWeight(float InT) {
    return max(vec3(1.0 - abs(InT - 0.0) * 2.0, 1.0 - abs(InT - 0.5) * 2.0, 1.0 - abs(InT - 1.0) * 2.0), vec3(0.0));
}

void main() {
    vec2 uv = gl_FragCoord.xy / vec2(textureSize(u_Source, 0));
    float aspect = u_CaScreen.x;

    // In units of the screen's height, so the fringes are as wide horizontally as vertically.
    vec2 fromCenter = (uv - 0.5) * vec2(aspect, 1.0);
    float cornerDistance = length(vec2(aspect, 1.0) * 0.5);
    float radius = length(fromCenter) / cornerDistance;
    float ramp = clamp((radius - u_CaParams.y) / (1.0 - u_CaParams.y), 0.0, 1.0);
    float amount = pow(ramp, u_CaParams.z) * u_CaParams.x;
    if (amount <= 0.0) {
        outColor = vec4(texture(u_Source, uv).rgb, 1.0);
        return;
    }

    vec2 direction = fromCenter / max(length(fromCenter), 1e-5) * amount / vec2(aspect, 1.0);
    int sampleCount = int(u_CaParams.w);
    vec3 sum = vec3(0.0);
    vec3 weightSum = vec3(0.0);
    for (int i = 0; i < sampleCount; i++) {
        float t = float(i) / float(sampleCount - 1);
        vec3 weight = SpectrumWeight(t);
        sum += texture(u_Source, uv - direction * (t - 0.5)).rgb * weight;
        weightSum += weight;
    }
    outColor = vec4(sum / max(weightSum, vec3(1e-4)), 1.0);
}

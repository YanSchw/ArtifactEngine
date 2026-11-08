#blend Opaque
#depth None
#cull None

#type vert
layout(location = 0) in vec3 a_Position;

void main() {
    gl_Position = vec4(a_Position, 1.0);
}

#type frag
#include "/Shaders/PostProcess/BloomCommon.glsl"

layout(location = 0) out vec4 outColor;

void main() {
    vec2 uv = DownsampledUV();
    vec3 color = Downsample(uv, true);

    // Quadratic soft knee around the threshold.
    float brightness = Brightness(color);
    float threshold = u_BloomParams.x;
    float knee = u_BloomParams.y;
    float soft = clamp(brightness - threshold + knee, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 1e-4);
    float contribution = max(soft, brightness - threshold) / max(brightness, 1e-4);
    outColor = vec4(color * max(contribution, 0.0), 1.0);
}

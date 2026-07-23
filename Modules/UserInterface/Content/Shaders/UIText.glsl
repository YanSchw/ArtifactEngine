#type vert
#version 450

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_UV;

layout(location = 1) out vec4 v_Color;
layout(location = 2) out vec2 v_UV;

layout(binding = 0) uniform UIProjection { mat4 u_Projection; };

void main() {
    gl_Position = u_Projection * vec4(a_Position, 1.0);
    v_Color = a_Color;
    v_UV = a_UV;
}

#type frag
#version 450
layout(location = 0) out vec4 outColor;

layout(location = 1) in vec4 v_Color;
layout(location = 2) in vec2 v_UV;

layout(binding = 16) uniform sampler2D u_Atlas;

// Distance-field range in atlas texels; MUST match s_MsdfPxRange in Font.cpp.
const float pxRange = 4.0;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

// Width of the distance field's transition band measured in screen pixels at this fragment,
// which keeps the antialiased edge ~1px wide at any text size.
float screenPxRange() {
    vec2 unitRange = vec2(pxRange) / vec2(textureSize(u_Atlas, 0));
    vec2 screenTexSize = vec2(1.0) / fwidth(v_UV);
    return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

void main() {
    vec3 msd = texture(u_Atlas, v_UV).rgb;
    float dist = median(msd.r, msd.g, msd.b);
    float screenPxDist = screenPxRange() * (dist - 0.5);
    float alpha = clamp(screenPxDist + 0.5, 0.0, 1.0);
    if (alpha <= 0.0) {
        discard;
    }
    outColor = vec4(v_Color.rgb, v_Color.a * alpha);
}

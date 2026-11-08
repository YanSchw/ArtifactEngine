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

// u_Source is the mip being upsampled, u_Detail the finer mip it is merged into.
layout(binding = 17) uniform sampler2D u_Detail;

layout(location = 0) out vec4 outColor;

void main() {
    vec2 uv = gl_FragCoord.xy / vec2(textureSize(u_Detail, 0));
    vec3 detail = texture(u_Detail, uv).rgb;
    vec3 coarse = UpsampleTent(u_Source, uv);
    outColor = vec4(mix(detail, coarse, u_BloomParams.w), 1.0);
}

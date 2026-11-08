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

layout(binding = 17) uniform sampler2D u_Bloom;

layout(location = 0) out vec4 outColor;

void main() {
    ivec2 texel = ivec2(gl_FragCoord.xy);
    vec2 uv = gl_FragCoord.xy / vec2(textureSize(u_Source, 0));
    vec3 color = texelFetch(u_Source, texel, 0).rgb;
    outColor = vec4(color + UpsampleTent(u_Bloom, uv) * u_BloomTint.rgb, 1.0);
}

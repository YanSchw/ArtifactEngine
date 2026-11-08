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
    outColor = vec4(Downsample(uv, false), 1.0);
}

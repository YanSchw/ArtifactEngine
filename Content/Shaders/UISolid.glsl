#type vert
#version 450

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Color;
layout(location = 2) in vec2 a_UV;

layout(location = 1) out vec4 v_Color;
layout(location = 2) out vec2 v_UV;

// Projects world pixel-space (with depth from tilt) to clip space, with perspective.
layout(binding = 0) uniform UIProjection { mat4 u_Projection; };

void main() {
    gl_Position = u_Projection * vec4(a_Position, 1.0);
    v_Color = vec4(a_Color, 1.0);
    v_UV = a_UV;
}

#type frag
#version 450
layout(location = 0) out vec4 outColor;

layout(location = 1) in vec4 v_Color;
layout(location = 2) in vec2 v_UV;

// A 1x1 white texture is bound here; sampling it yields alpha 1 so rects are opaque.
layout(binding = 16) uniform sampler2D u_Texture;

void main() {
    float a = texture(u_Texture, v_UV).a;
    outColor = vec4(v_Color.rgb, a);
}

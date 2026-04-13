#type vert
#version 450

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Color;
layout(location = 2) in vec2 a_UV;

layout(location = 1) out vec4 v_Color;
layout(location = 2) out vec2 v_UV;

void main() {
    gl_Position = vec4(a_Position, 1.0);
    v_Color = vec4(a_Color, 1.0);
    v_UV = a_UV;
}

#type frag
#version 450
layout(location = 0) out vec4 outColor;

layout(location = 1) in vec4 v_Color;
layout(location = 2) in vec2 v_UV;

layout(binding = 16) uniform sampler2D u_Texture;

void main() {
    outColor = texture(u_Texture, v_UV);
}
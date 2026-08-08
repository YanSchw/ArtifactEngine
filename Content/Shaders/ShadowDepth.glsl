#cull None
#depth TestWrite

#type vert
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Color;
layout(location = 2) in vec2 a_UV;
layout(location = 3) in vec3 a_Normal;

layout(binding = 0, std140) uniform CascadeBlock {
    mat4 u_LightViewProjection;
};

layout(push_constant) uniform ShaderDataBlock {
    mat4 WorldTransform;
    uint NodeId;
} u_ShaderData;

void main() {
    gl_Position = u_LightViewProjection * u_ShaderData.WorldTransform * vec4(a_Position, 1.0);
}

#type frag

void main() {
}

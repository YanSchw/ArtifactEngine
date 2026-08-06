#blend Alpha
#depth None
#cull None

#type vert
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;

layout(binding = 0) uniform UBO {
    mat4 u_ViewProjection;
};

layout(location = 1) out vec4 v_Color;

void main() {
    gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
    v_Color = a_Color;
}

#type frag
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNodeId;

layout(location = 1) in vec4 v_Color;

void main() {
    outColor = v_Color;
    outNodeId = vec4(0.0);
}

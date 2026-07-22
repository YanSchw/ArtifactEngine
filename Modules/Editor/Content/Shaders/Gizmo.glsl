#type vert
#version 450

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Color;
layout(location = 2) in vec2 a_UV;

layout(binding = 0) uniform UBO {
    mat4 u_ViewProjection;
};

layout(push_constant) uniform GizmoData {
    mat4 WorldTransform;
    vec4 Color;
    uint NodeId;
} u_Gizmo;

layout(location = 1) out vec3 v_WorldPos;

void main() {
    vec4 worldPos = u_Gizmo.WorldTransform * vec4(a_Position, 1.0);
    gl_Position = u_ViewProjection * worldPos;
    v_WorldPos = worldPos.xyz;
}

#type frag
#version 450

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNodeId;

layout(location = 1) in vec3 v_WorldPos;

layout(push_constant) uniform GizmoData {
    mat4 WorldTransform;
    vec4 Color;
    uint NodeId;
} u_Gizmo;

void main() {
    // Gizmo meshes carry no normals, so shade from the derivatives of world position.
    vec3 normal = normalize(cross(dFdx(v_WorldPos), dFdy(v_WorldPos)));
    float lambert = max(dot(normal, normalize(vec3(0.4, 0.85, 0.35))), 0.0);
    outColor = vec4(u_Gizmo.Color.rgb * (0.45 + 0.55 * lambert), u_Gizmo.Color.a);

    uint id = u_Gizmo.NodeId;
    outNodeId = vec4(float(id & 0xFFu),
                     float((id >> 8) & 0xFFu),
                     float((id >> 16) & 0xFFu),
                     float((id >> 24) & 0xFFu)) / 255.0;
}

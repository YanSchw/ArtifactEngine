#include "/Shaders/Common/Scene.glsl"

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Color;
layout(location = 2) in vec2 a_UV;
layout(location = 3) in vec3 a_Normal;

layout(location = 4) in vec3 i_Position;
layout(location = 5) in vec3 i_Scale;
layout(location = 6) in vec4 i_Rotation;
layout(location = 7) in vec4 i_Color;
layout(location = 8) in vec4 i_Params;

layout(location = 0) out vec4 v_Color;
layout(location = 1) out vec2 v_UV;
layout(location = 2) out vec3 v_Normal;
layout(location = 3) out vec3 v_BlendUV;
layout(location = 4) out vec4 v_Params;
layout(location = 5) out vec3 v_WorldPosition;

vec3 Rotate(vec4 InRotation, vec3 InVector) {
    return InVector + 2.0 * cross(InRotation.xyz, cross(InRotation.xyz, InVector) + InRotation.w * InVector);
}

void main() {
    vec3 worldPosition = i_Position + Rotate(i_Rotation, a_Position * i_Scale);
    gl_Position = u_ViewProjection * vec4(worldPosition, 1.0);
    v_Color = i_Color;
    v_UV = a_UV;
    v_Normal = normalize(Rotate(i_Rotation, a_Normal / max(abs(i_Scale), vec3(1e-6))));
    v_BlendUV = vec3(a_UV, 0.0);
    v_Params = i_Params;
    v_WorldPosition = worldPosition;
}

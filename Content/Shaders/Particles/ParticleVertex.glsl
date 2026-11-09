#include "/Shaders/Common/Scene.glsl"

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_UV;
layout(location = 3) in vec3 a_Normal;
layout(location = 4) in vec3 a_BlendUV;
layout(location = 5) in vec4 a_Params;

layout(location = 0) out vec4 v_Color;
layout(location = 1) out vec2 v_UV;
layout(location = 2) out vec3 v_Normal;
layout(location = 3) out vec3 v_BlendUV;
layout(location = 4) out vec4 v_Params;
layout(location = 5) out vec3 v_WorldPosition;

void main() {
    gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
    v_Color = a_Color;
    v_UV = a_UV;
    v_Normal = a_Normal;
    v_BlendUV = a_BlendUV;
    v_Params = a_Params;
    v_WorldPosition = a_Position;
}

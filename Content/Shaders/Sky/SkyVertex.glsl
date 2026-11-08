
#include "/Shaders/Common/Scene.glsl"

layout(location = 0) in vec3 a_Position;

layout(location = 0) out vec4 v_Near;
layout(location = 1) out vec4 v_Far;

void main() {
    gl_Position = vec4(a_Position.xy, 1.0, 1.0);
    v_Near = u_InverseViewProjection * vec4(a_Position.xy, 0.0, 1.0);
    v_Far = u_InverseViewProjection * vec4(a_Position.xy, 1.0, 1.0);
}

#blend Additive
#depth None
#cull None

#type vert
layout(location = 0) in vec3 a_Position;

void main() {
    gl_Position = vec4(a_Position, 1.0);
}

#type frag
#include "/Shaders/Deferred/Common.glsl"
#include "/Shaders/Common/Lighting.glsl"

void main() {
    GBufferTexel texel;
    if (!ReadGBuffer(texel)) {
        discard;
    }
    vec3 light = SunLighting(texel.WorldPosition, texel.Normal) + u_AmbientColor.rgb;
    outColor = vec4(texel.Albedo * light, 1.0);
    outNodeId = vec4(0.0);
}

#blend Opaque
#depth TestWrite
#cull None

#type vert
layout(location = 0) in vec3 a_Position;

void main() {
    gl_Position = vec4(a_Position, 1.0);
}

#type frag
#include "/Shaders/Deferred/Common.glsl"

void main() {
    GBufferTexel texel;
    if (!ReadGBuffer(texel)) {
        discard;
    }
    gl_FragDepth = texel.Depth;
    outColor = vec4(texel.Emissive, 1.0);
    outNodeId = texel.NodeId;
}

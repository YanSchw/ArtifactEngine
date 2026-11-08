#blend Opaque
#depth Write
#cull None

#type vert
layout(location = 0) in vec3 a_Position;

void main() {
    gl_Position = vec4(a_Position, 1.0);
}

#type frag
layout(binding = 16) uniform sampler2D u_Color;
layout(binding = 17) uniform sampler2D u_NodeId;
layout(binding = 18) uniform sampler2D u_Depth;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNodeId;

void main() {
    ivec2 texel = ivec2(gl_FragCoord.xy);
    outColor = vec4(texelFetch(u_Color, texel, 0).rgb, 1.0);
    outNodeId = texelFetch(u_NodeId, texel, 0);
    gl_FragDepth = texelFetch(u_Depth, texel, 0).r;
}

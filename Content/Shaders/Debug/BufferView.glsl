#blend Opaque
#depth None
#cull None

#type vert
layout(location = 0) in vec3 a_Position;

void main() {
    gl_Position = vec4(a_Position, 1.0);
}

#type frag
// Mirrors BufferViewUniforms in RenderPipeline.cpp.
layout(binding = 1, std140) uniform BufferViewBlock {
    mat4 u_InverseProjection;
    // x: RenderBufferKind
    vec4 u_BufferViewParams;
};
layout(binding = 16) uniform sampler2D u_Buffer;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNodeId;

const float KindDepth = 1.0;
const float KindNodeId = 2.0;

float ViewDistance(float InDepth) {
    vec4 view = u_InverseProjection * vec4(0.0, 0.0, InDepth, 1.0);
    return abs(view.z / view.w);
}

vec3 DepthColor(float InDepth) {
    if (InDepth >= 1.0) {
        return vec3(0.0);
    }
    float nearDistance = max(ViewDistance(0.0), 1e-4);
    float farDistance = max(ViewDistance(1.0), nearDistance * 1.001);
    float viewDistance = clamp(ViewDistance(InDepth), nearDistance, farDistance);
    return vec3(1.0 - log(viewDistance / nearDistance) / log(farDistance / nearDistance));
}

vec3 NodeIdColor(vec4 InPacked) {
    uvec4 bytes = uvec4(round(InPacked * 255.0));
    uint id = bytes.x | (bytes.y << 8) | (bytes.z << 16) | (bytes.w << 24);
    if (id == 0u) {
        return vec3(0.0);
    }
    uint hash = id * 2654435761u;
    return vec3(uvec3(hash, hash >> 8, hash >> 16) & 255u) / 255.0 * 0.75 + 0.25;
}

void main() {
    vec4 texel = texelFetch(u_Buffer, ivec2(gl_FragCoord.xy), 0);
    vec3 color = texel.rgb;
    if (u_BufferViewParams.x == KindDepth) {
        color = DepthColor(texel.r);
    } else if (u_BufferViewParams.x == KindNodeId) {
        color = NodeIdColor(texel);
    }
    outColor = vec4(color, 1.0);
    outNodeId = vec4(0.0);
}

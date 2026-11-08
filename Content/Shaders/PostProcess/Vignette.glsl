#blend Opaque
#depth None
#cull None

#type vert
layout(location = 0) in vec3 a_Position;

void main() {
    gl_Position = vec4(a_Position, 1.0);
}

#type frag
// Mirrors VignetteUniforms in VignettePass.cpp.
layout(binding = 1, std140) uniform VignetteBlock {
    // x: intensity, y: smoothness exponent, z: roundness, w: aspect ratio
    vec4 u_VignetteParams;
    vec4 u_VignetteCenter;
    vec4 u_VignetteTint;
};
layout(binding = 16) uniform sampler2D u_Source;

layout(location = 0) out vec4 outColor;

void main() {
    ivec2 texel = ivec2(gl_FragCoord.xy);
    vec2 uv = gl_FragCoord.xy / vec2(textureSize(u_Source, 0));
    vec3 color = texelFetch(u_Source, texel, 0).rgb;

    vec2 offset = abs(uv - u_VignetteCenter.xy) * u_VignetteParams.x;
    offset.x *= mix(1.0, u_VignetteParams.w, u_VignetteParams.z);
    float visibility = pow(clamp(1.0 - dot(offset, offset), 0.0, 1.0), u_VignetteParams.y);
    outColor = vec4(color * mix(u_VignetteTint.rgb, vec3(1.0), visibility), 1.0);
}

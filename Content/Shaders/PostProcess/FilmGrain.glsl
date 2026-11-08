#blend Opaque
#depth None
#cull None

#type vert
layout(location = 0) in vec3 a_Position;

void main() {
    gl_Position = vec4(a_Position, 1.0);
}

#type frag
// Mirrors FilmGrainUniforms in FilmGrainPass.cpp.
layout(binding = 1, std140) uniform FilmGrainBlock {
    // x: intensity, y: grain size in pixels, z: luminance response, w: colorfulness
    vec4 u_GrainParams;
    // x: frame seed
    vec4 u_GrainSeed;
};
layout(binding = 16) uniform sampler2D u_Source;

layout(location = 0) out vec4 outColor;

float Hash(vec2 InPoint) {
    vec3 p = fract(vec3(InPoint.xyx) * 0.1031);
    p += dot(p, p.yzx + 33.33);
    return fract((p.x + p.y) * p.z);
}

// Smoothly interpolated value noise in -1..1, so a grain spans more than one pixel without blocks.
float GrainNoise(vec2 InPoint) {
    vec2 cell = floor(InPoint);
    vec2 f = InPoint - cell;
    f = f * f * (3.0 - 2.0 * f);
    float a = Hash(cell);
    float b = Hash(cell + vec2(1.0, 0.0));
    float c = Hash(cell + vec2(0.0, 1.0));
    float d = Hash(cell + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y) * 2.0 - 1.0;
}

// Overlay blend, which leaves pure black and white alone and is strongest in the mid tones.
vec3 Overlay(vec3 InBase, vec3 InGrain) {
    vec3 blend = 0.5 + 0.5 * InGrain;
    vec3 dark = 2.0 * InBase * blend;
    vec3 light = 1.0 - 2.0 * (1.0 - InBase) * (1.0 - blend);
    return mix(dark, light, step(0.5, InBase));
}

void main() {
    ivec2 texel = ivec2(gl_FragCoord.xy);
    vec3 color = clamp(texelFetch(u_Source, texel, 0).rgb, 0.0, 1.0);

    vec2 offset = fract(u_GrainSeed.x * vec2(0.7548776662, 0.5698402910)) * 512.0;
    vec2 point = gl_FragCoord.xy / u_GrainParams.y + offset;
    float mono = GrainNoise(point);
    vec3 chroma = vec3(mono, GrainNoise(point + vec2(97.3, 41.7)), GrainNoise(point + vec2(23.1, 157.9)));
    vec3 grain = mix(vec3(mono), chroma, u_GrainParams.w);

    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float response = mix(1.0, 1.0 - sqrt(luma), u_GrainParams.z);
    outColor = vec4(Overlay(color, grain * u_GrainParams.x * response), 1.0);
}

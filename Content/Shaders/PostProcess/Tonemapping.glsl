#blend Opaque
#depth None
#cull None

#type vert
layout(location = 0) in vec3 a_Position;

void main() {
    gl_Position = vec4(a_Position, 1.0);
}

#type frag
layout(binding = 1, std140) uniform TonemappingBlock {
    vec4 u_TonemappingParams;
};
layout(binding = 16) uniform sampler2D u_Source;

layout(location = 0) out vec4 outColor;

const int ModeLinear = 1;
const int ModeReinhard = 2;
const int ModeHable = 3;
const int ModeAces = 4;
const int ModeAgX = 5;

float Luminance(vec3 InColor) {
    return dot(InColor, vec3(0.2126, 0.7152, 0.0722));
}

vec3 Reinhard(vec3 InColor, float InWhite) {
    float luminance = Luminance(InColor);
    float mapped = luminance * (1.0 + luminance / (InWhite * InWhite)) / (1.0 + luminance);
    return InColor * (mapped / max(luminance, 1e-6));
}

// John Hable's Uncharted 2 filmic curve.
vec3 HableCurve(vec3 InColor) {
    const float A = 0.15, B = 0.50, C = 0.10, D = 0.20, E = 0.02, F = 0.30;
    return ((InColor * (A * InColor + C * B) + D * E) / (InColor * (A * InColor + B) + D * F)) - E / F;
}

vec3 Hable(vec3 InColor) {
    const float ExposureBias = 2.0;
    const float LinearWhite = 11.2;
    return HableCurve(InColor * ExposureBias) / HableCurve(vec3(LinearWhite));
}

// Stephen Hill's fit of the ACES reference rendering and sRGB output transforms.
vec3 Aces(vec3 InColor) {
    const mat3 InputMatrix = mat3(0.59719, 0.07600, 0.02840,
                                  0.35458, 0.90834, 0.13383,
                                  0.04823, 0.01566, 0.83777);
    const mat3 OutputMatrix = mat3(1.60475, -0.10208, -0.00327,
                                   -0.53108, 1.10813, -0.07276,
                                   -0.07367, -0.00605, 1.07602);
    vec3 color = InputMatrix * InColor;
    color = (color * (color + 0.0245786) - 0.000090537) / (color * (0.983729 * color + 0.4329510) + 0.238081);
    return OutputMatrix * color;
}

// Troy Sobotka's AgX, after Benjamin Wrensch's polynomial approximation of its base contrast.
vec3 AgX(vec3 InColor) {
    const mat3 Inset = mat3(0.842479062253094, 0.0423282422610123, 0.0423756549057051,
                            0.0784335999999992, 0.878468636469772, 0.0784336,
                            0.0792237451477643, 0.0791661274605434, 0.879142973793104);
    const mat3 Outset = mat3(1.19687900512017, -0.0528968517574562, -0.0529716355144438,
                             -0.0980208811401368, 1.15190312990417, -0.0980434501171241,
                             -0.0990297440797205, -0.0989611768448433, 1.15107367264116);
    const float MinEv = -12.47393;
    const float MaxEv = 4.026069;

    vec3 color = Inset * InColor;
    color = clamp((log2(max(color, vec3(1e-10))) - MinEv) / (MaxEv - MinEv), 0.0, 1.0);
    vec3 x2 = color * color;
    vec3 x4 = x2 * x2;
    color = 15.5 * x4 * x2 - 40.14 * x4 * color + 31.96 * x4 - 6.868 * x2 * color
          + 0.4298 * x2 + 0.1191 * color - 0.00232;
    return pow(max(Outset * color, vec3(0.0)), vec3(2.2));
}

// The Khronos PBR Neutral curve: leaves colors untouched up to 0.76 and only rolls off highlights.
vec3 Neutral(vec3 InColor) {
    const float StartCompression = 0.8 - 0.04;
    const float Desaturation = 0.15;
    float darkest = min(InColor.r, min(InColor.g, InColor.b));
    vec3 color = InColor - (darkest < 0.08 ? darkest - 6.25 * darkest * darkest : 0.04);
    float peak = max(color.r, max(color.g, color.b));
    if (peak < StartCompression) {
        return color;
    }
    const float D = 1.0 - StartCompression;
    float newPeak = 1.0 - D * D / (peak + D - StartCompression);
    color *= newPeak / peak;
    float blend = 1.0 - 1.0 / (Desaturation * (peak - newPeak) + 1.0);
    return mix(color, vec3(newPeak), blend);
}

vec3 Tonemap(vec3 InColor, int InMode) {
    if (InMode == ModeLinear) {
        return InColor;
    } else if (InMode == ModeReinhard) {
        return Reinhard(InColor, u_TonemappingParams.z);
    } else if (InMode == ModeHable) {
        return Hable(InColor);
    } else if (InMode == ModeAces) {
        return Aces(InColor);
    } else if (InMode == ModeAgX) {
        return AgX(InColor);
    }
    return Neutral(InColor);
}

vec3 LinearToSrgb(vec3 InColor) {
    return mix(InColor * 12.92, 1.055 * pow(InColor, vec3(1.0 / 2.4)) - 0.055, step(0.0031308, InColor));
}

void main() {
    vec3 color = max(texelFetch(u_Source, ivec2(gl_FragCoord.xy), 0).rgb, vec3(0.0)) * u_TonemappingParams.x;
    vec3 mapped = clamp(Tonemap(color, int(u_TonemappingParams.y + 0.5)), 0.0, 1.0);
    outColor = vec4(LinearToSrgb(mapped), 1.0);
}

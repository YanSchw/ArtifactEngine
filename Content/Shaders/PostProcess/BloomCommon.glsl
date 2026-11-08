#pragma once

// Mirrors BloomUniforms in BloomPass.cpp.
layout(binding = 1, std140) uniform BloomBlock {
    // x: threshold, y: knee, z: max brightness, w: scatter
    vec4 u_BloomParams;
    // rgb: tint already scaled by the intensity
    vec4 u_BloomTint;
};

layout(binding = 16) uniform sampler2D u_Source;

float Brightness(vec3 InColor) {
    return max(InColor.r, max(InColor.g, InColor.b));
}

// With InKaris each 2x2 group is weighted by its inverse luma, which stops single hot pixels from flickering.
vec3 Downsample(vec2 InUV, bool InKaris) {
    vec2 texel = 1.0 / vec2(textureSize(u_Source, 0));
    float maxBrightness = u_BloomParams.z;
    vec3 a = min(texture(u_Source, InUV + texel * vec2(-2.0, 2.0)).rgb, vec3(maxBrightness));
    vec3 b = min(texture(u_Source, InUV + texel * vec2(0.0, 2.0)).rgb, vec3(maxBrightness));
    vec3 c = min(texture(u_Source, InUV + texel * vec2(2.0, 2.0)).rgb, vec3(maxBrightness));
    vec3 d = min(texture(u_Source, InUV + texel * vec2(-2.0, 0.0)).rgb, vec3(maxBrightness));
    vec3 e = min(texture(u_Source, InUV).rgb, vec3(maxBrightness));
    vec3 f = min(texture(u_Source, InUV + texel * vec2(2.0, 0.0)).rgb, vec3(maxBrightness));
    vec3 g = min(texture(u_Source, InUV + texel * vec2(-2.0, -2.0)).rgb, vec3(maxBrightness));
    vec3 h = min(texture(u_Source, InUV + texel * vec2(0.0, -2.0)).rgb, vec3(maxBrightness));
    vec3 i = min(texture(u_Source, InUV + texel * vec2(2.0, -2.0)).rgb, vec3(maxBrightness));
    vec3 j = min(texture(u_Source, InUV + texel * vec2(-1.0, 1.0)).rgb, vec3(maxBrightness));
    vec3 k = min(texture(u_Source, InUV + texel * vec2(1.0, 1.0)).rgb, vec3(maxBrightness));
    vec3 l = min(texture(u_Source, InUV + texel * vec2(-1.0, -1.0)).rgb, vec3(maxBrightness));
    vec3 m = min(texture(u_Source, InUV + texel * vec2(1.0, -1.0)).rgb, vec3(maxBrightness));

    vec3 groups[5] = vec3[](
        (j + k + l + m) * 0.25,
        (a + b + d + e) * 0.25,
        (b + c + e + f) * 0.25,
        (d + e + g + h) * 0.25,
        (e + f + h + i) * 0.25
    );
    float weights[5] = float[](0.5, 0.125, 0.125, 0.125, 0.125);

    vec3 result = vec3(0.0);
    float weightSum = 0.0;
    for (int group = 0; group < 5; group++) {
        float weight = weights[group];
        if (InKaris) {
            weight /= 1.0 + dot(groups[group], vec3(0.2126, 0.7152, 0.0722));
        }
        result += groups[group] * weight;
        weightSum += weight;
    }
    return result / weightSum;
}

// Mirrors BloomPass, which halves every mip rounding up.
vec2 DownsampledUV() {
    return gl_FragCoord.xy / vec2(max((textureSize(u_Source, 0) + 1) / 2, ivec2(1)));
}

// 3x3 tent over InImage's texels.
vec3 UpsampleTent(sampler2D InImage, vec2 InUV) {
    vec2 texel = 1.0 / vec2(textureSize(InImage, 0));
    vec3 result = texture(InImage, InUV).rgb * 4.0;
    result += (texture(InImage, InUV + vec2(texel.x, 0.0)).rgb + texture(InImage, InUV - vec2(texel.x, 0.0)).rgb
             + texture(InImage, InUV + vec2(0.0, texel.y)).rgb + texture(InImage, InUV - vec2(0.0, texel.y)).rgb) * 2.0;
    result += texture(InImage, InUV + texel).rgb + texture(InImage, InUV - texel).rgb
            + texture(InImage, InUV + vec2(texel.x, -texel.y)).rgb + texture(InImage, InUV + vec2(-texel.x, texel.y)).rgb;
    return result / 16.0;
}

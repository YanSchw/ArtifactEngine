#pragma once

// Expects the SceneBlock and u_ShadowMap declared by #gen_buffers(frag).

#include "/Shaders/Common/Lighting.glsl"

struct CelSurface {
    vec3 BaseColor;
    vec3 Normal;
    vec3 WorldPosition;

    vec3 ShadowColor;
    float ShadeThreshold;
    float ShadeSoftness;

    vec3 SpecularColor;
    float SpecularStrength;
    float SpecularSmoothness;

    vec3 RimColor;
    float RimStrength;
    float RimWidth;
};

/** Every edge in this model is one of these, so a single Softness reads as one drawing style
 *  across the terminator, the cast shadow, the highlight and the rim. */
float CelBand(float InValue, float InThreshold, float InSoftness) {
    float softness = max(InSoftness, 1e-3);
    return smoothstep(InThreshold - softness, InThreshold + softness, InValue);
}

vec3 CelSaturate(vec3 InColor, float InAmount) {
    float luminance = dot(InColor, vec3(0.2126, 0.7152, 0.0722));
    return max(mix(vec3(luminance), InColor, InAmount), vec3(0.0));
}

vec3 CelPointLighting(CelSurface InSurface, vec3 InNormal) {
    vec3 result = vec3(0.0);
    int count = int(u_PointLightCount);
    for (int i = 0; i < count; i++) {
        vec3 toLight = u_PointLightPositions[i].xyz - InSurface.WorldPosition;
        float distanceSquared = max(dot(toLight, toLight), 1e-4);
        float radius = max(u_PointLightPositions[i].w, 1e-3);

        float window = clamp(1.0 - (distanceSquared * distanceSquared) / pow(radius, 4.0), 0.0, 1.0);
        float attenuation = (window * window) / distanceSquared;
        float band = CelBand(dot(InNormal, toLight * inversesqrt(distanceSquared)),
                             InSurface.ShadeThreshold, InSurface.ShadeSoftness);

        result += u_PointLightColors[i].rgb * band * attenuation;
    }
    return result;
}

/** Two-tone sun light: a lit color and a shade color, chosen per surface and cut against each other
 *  at the terminator, with the cast shadow cut by the same edge so both read as one drawing. */
vec3 ShadeCel(CelSurface InSurface) {
    vec3 normal = normalize(InSurface.Normal);
    vec3 toSun = -u_SunDirection.xyz;
    vec3 toEye = normalize(u_CameraPosition.xyz - InSurface.WorldPosition);

    float lambert = dot(normal, toSun);
    float sunTerm = CelBand(lambert, InSurface.ShadeThreshold, InSurface.ShadeSoftness);
    sunTerm *= CelBand(SunShadow(InSurface.WorldPosition, normal), 0.5, max(InSurface.ShadeSoftness, 0.08));

    // The sky reaches upward faces and not much reaches the undersides, which is the whole of the
    // ambient occlusion this model has and most of what gives an unlit side any form at all.
    vec3 skyLight = u_AmbientColor.rgb * mix(0.35, 1.0, normal.y * 0.5 + 0.5);

    // Two painted colors picked per surface, not one color dimmed: the shade keeps the hue and
    // gains saturation as it darkens, and only it collects the ambient, so the lit band lands on
    // the base color instead of clipping past it and taking the highlight with it.
    vec3 lit = InSurface.BaseColor * u_SunColor.rgb;
    vec3 shade = CelSaturate(InSurface.BaseColor * InSurface.ShadowColor, 1.2)
               * u_SunColor.rgb * mix(0.82, 1.0, lambert * 0.5 + 0.5)
               + InSurface.BaseColor * skyLight;

    vec3 diffuse = mix(shade, lit, sunTerm)
                 + InSurface.BaseColor * CelPointLighting(InSurface, normal);

    float gloss = exp2(mix(1.0, 9.0, clamp(InSurface.SpecularSmoothness, 0.0, 1.0)));
    float highlight = pow(max(dot(normal, normalize(toSun + toEye)), 0.0), gloss);
    vec3 specular = InSurface.SpecularColor * u_SunColor.rgb * InSurface.SpecularStrength
                  * CelBand(highlight, 0.5, InSurface.ShadeSoftness) * sunTerm;

    // A rim only the sun can light reads as a back light; one that survives into the shade reads
    // as sky bounce. Keeping most of it lets a silhouette hold up against a dark background.
    float edge = 1.0 - max(dot(normal, toEye), 0.0);
    vec3 rim = InSurface.RimColor * InSurface.RimStrength
             * CelBand(edge, 1.0 - clamp(InSurface.RimWidth, 0.0, 1.0), InSurface.ShadeSoftness * 3.0)
             * mix(0.35, 1.0, sunTerm);

    return diffuse + specular + rim;
}

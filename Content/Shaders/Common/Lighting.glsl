#pragma once

// Expects the SceneBlock and u_ShadowMap declared by #gen_buffers(frag).

float SampleCascade(int InCascade, vec2 InUV, float InDepth) {
    float tapStep = u_ShadowParams.w;
    float lit = 0.0;
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            vec2 offset = (vec2(x, y) - 1.5) * tapStep;
            lit += texture(u_ShadowMap, vec4(InUV + offset, float(InCascade), InDepth));
        }
    }
    return lit / 16.0;
}

/** 1 where the sun reaches InWorldPosition, 0 where it is fully occluded. The cascades are ordered
 *  near to far, so the first one the point falls into is also the sharpest one covering it. */
float SunShadow(vec3 InWorldPosition, vec3 InNormal) {
    int cascadeCount = int(u_ShadowParams.z);
    // Keeping the whole filter kernel inside the cascade, rather than only its center, is what
    // stops the border from clamping its outermost taps.
    float border = 1.0 - 3.0 * u_ShadowParams.w;

    for (int cascade = 0; cascade < cascadeCount; cascade++) {
        vec3 offsetPosition = InWorldPosition + InNormal * (u_ShadowParams.y * u_CascadeTexelSizes[cascade]);
        vec4 shadowClip = u_ShadowMatrices[cascade] * vec4(offsetPosition, 1.0);
        vec3 projected = shadowClip.xyz / shadowClip.w;

        if (any(greaterThan(abs(projected.xy), vec2(border))) || projected.z < 0.0 || projected.z > 1.0) {
            continue;
        }
        return SampleCascade(cascade, projected.xy * 0.5 + 0.5, projected.z - u_ShadowParams.x);
    }
    return 1.0;
}

vec3 PointLighting(vec3 InWorldPosition, vec3 InNormal) {
    vec3 result = vec3(0.0);
    int count = int(u_PointLightCount);
    for (int i = 0; i < count; i++) {
        vec3 toLight = u_PointLightPositions[i].xyz - InWorldPosition;
        float distanceSquared = max(dot(toLight, toLight), 1e-4);
        float radius = max(u_PointLightPositions[i].w, 1e-3);

        float window = clamp(1.0 - (distanceSquared * distanceSquared) / pow(radius, 4.0), 0.0, 1.0);
        float attenuation = (window * window) / distanceSquared;
        float lambert = max(dot(InNormal, toLight * inversesqrt(distanceSquared)), 0.0);

        result += u_PointLightColors[i].rgb * lambert * attenuation;
    }
    return result;
}

vec3 ShadeSurface(vec3 InBaseColor, vec3 InNormal, vec3 InWorldPosition) {
    vec3 normal = normalize(InNormal);

    float sunLambert = max(dot(normal, -u_SunDirection.xyz), 0.0);
    vec3 light = u_SunColor.rgb * sunLambert * SunShadow(InWorldPosition, normal);
    light += PointLighting(InWorldPosition, normal);

    return InBaseColor * (light + u_AmbientColor.rgb);
}

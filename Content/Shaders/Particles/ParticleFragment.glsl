#include "/Shaders/Common/Scene.glsl"

layout(binding = 2) uniform sampler2DArrayShadow u_ShadowMap;
layout(binding = 16) uniform sampler2D u_Texture;
layout(binding = 17) uniform sampler2D u_SceneDepth;

#include "/Shaders/Common/Lighting.glsl"

layout(location = 0) in vec4 v_Color;
layout(location = 1) in vec2 v_UV;
layout(location = 2) in vec3 v_Normal;
layout(location = 3) in vec3 v_BlendUV;
layout(location = 4) in vec4 v_Params;
layout(location = 5) in vec3 v_WorldPosition;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNodeId;

float SoftFade(float InDistance, float InSoftness) {
    float depth = texelFetch(u_SceneDepth, ivec2(gl_FragCoord.xy), 0).r;
    if (InSoftness <= 0.0 || depth >= 1.0) {
        return 1.0;
    }
    vec2 ndc = gl_FragCoord.xy / vec2(textureSize(u_SceneDepth, 0)) * 2.0 - 1.0;
    vec4 scene = u_InverseViewProjection * vec4(ndc, depth, 1.0);
    return clamp((distance(scene.xyz / scene.w, u_CameraPosition.xyz) - InDistance) / InSoftness, 0.0, 1.0);
}

void main() {
    vec4 color = texture(u_Texture, v_UV);
    if (v_BlendUV.z > 0.0) {
        color = mix(color, texture(u_Texture, v_BlendUV.xy), v_BlendUV.z);
    }
    color *= v_Color;

    if (v_Params.w > 0.0) {
        vec3 normal = normalize(v_Normal);
        color.rgb *= SunLighting(v_WorldPosition, normal) + PointLighting(v_WorldPosition, normal) + AmbientLighting(normal);
    }

    float cameraDistance = distance(v_WorldPosition, u_CameraPosition.xyz);
    float fade = SoftFade(cameraDistance, v_Params.x);
    if (v_Params.z > v_Params.y) {
        fade *= clamp((cameraDistance - v_Params.y) / (v_Params.z - v_Params.y), 0.0, 1.0);
    }

#if defined(PARTICLE_PREMULTIPLIED)
    outColor = vec4(color.rgb * v_Color.a, color.a) * fade;
    outNodeId = vec4(0.0);
#elif defined(PARTICLE_MULTIPLY)
    outColor = vec4(mix(vec3(1.0), color.rgb, color.a * fade), 1.0);
    outNodeId = vec4(1.0);
#else
    outColor = vec4(color.rgb, color.a * fade);
    outNodeId = vec4(0.0);
#endif
}

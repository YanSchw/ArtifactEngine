#shadergraph "Surface"

#blend (Opaque, Alpha, Additive, Multiply)
#cull (Back, Front, None)
#depth (TestWrite, Test, Write, None)
#renderpath (Deferred, Forward)
#frontface Clockwise

#type vert
#gen_buffers(vert)

void main() {
    vec4 worldPosition = u_ShaderData.WorldTransform * vec4(a_Position, 1.0);
    gl_Position = u_ViewProjection * worldPosition;

    v_Color = a_Color;
    v_UV = a_UV;
    v_WorldPosition = worldPosition.xyz;
    v_Normal = mat3(transpose(inverse(u_ShaderData.WorldTransform))) * a_Normal;
}

#type frag
#gen_buffers(frag)
#include "/Shaders/Common/Lighting.glsl"
#include "/Shaders/Common/Encoding.glsl"

// 8x8 ordered dither threshold in (0, 1)
float DitherThreshold(vec2 InFragCoord) {
    uvec2 pixel = uvec2(InFragCoord) & 7u;
    uint v = pixel.x ^ pixel.y;
    uint rank = ((v & 1u) << 5) | ((pixel.y & 1u) << 4)
              | ((v & 2u) << 2) | ((pixel.y & 2u) << 1)
              | ((v & 4u) >> 1) | ((pixel.y & 4u) >> 2);
    return (float(rank) + 0.5) / 64.0;
}

void main() {
    #property(BaseColor_, "BaseColor", Color, vec4(0.8, 0.8, 0.8, 1.0))
    #property(Emissive_, "Emissive", Color, vec4(0.0, 0.0, 0.0, 1.0))
    #property(Opacity_, "Opacity", float, 1.0)

    float opacity = BaseColor_.a * Opacity_;

#ifdef SG_DEFERRED
    if (opacity < DitherThreshold(gl_FragCoord.xy)) {
        discard;
    }
    outAlbedo = vec4(BaseColor_.rgb, 1.0);
    outNormal = vec4(EncodeNormal(v_Normal), 1.0);
    outEmissive = vec4(Emissive_.rgb, 1.0);
#else
    vec3 shaded = ShadeSurface(BaseColor_.rgb, v_Normal, v_WorldPosition);
    outColor = vec4(shaded + Emissive_.rgb, opacity);
#endif
    outNodeId = PackNodeId(u_ShaderData.NodeId);
}

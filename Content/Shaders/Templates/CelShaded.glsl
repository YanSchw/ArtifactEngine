#shadergraph "Cel Shaded"

#blend (Opaque, Alpha, Additive, Multiply)
#cull (Back, Front, None)
#depth (TestWrite, Test, Write, None)
#frontface Clockwise

#type vert
#gen_buffers(vert)

void main() {
    vec4 worldPosition = u_ShaderData.WorldTransform * vec4(a_Position, 1.0);
    gl_Position = u_ViewProjection * worldPosition;

    v_Color = vec4(a_Color, 1.0);
    v_UV = a_UV;
    v_WorldPosition = worldPosition.xyz;
    v_Normal = mat3(transpose(inverse(u_ShaderData.WorldTransform))) * a_Normal;
}

#type frag
#gen_buffers(frag)
#include "/Shaders/Common/CelLighting.glsl"

void main() {
    #property(BaseColor_, "BaseColor", Color, vec4(0.8, 0.8, 0.8, 1.0))
    #property(ShadowColor_, "ShadowColor", Color, vec4(0.55, 0.6, 0.72, 1.0))
    #property(ShadeThreshold_, "ShadeThreshold", float, 0.1)
    #property(ShadeSoftness_, "ShadeSoftness", float, 0.04)
    #property(SpecularColor_, "SpecularColor", Color, vec4(1.0, 1.0, 1.0, 1.0))
    #property(SpecularStrength_, "SpecularStrength", float, 0.25)
    #property(SpecularSmoothness_, "SpecularSmoothness", float, 0.5)
    #property(RimColor_, "RimColor", Color, vec4(1.0, 0.97, 0.9, 1.0))
    #property(RimStrength_, "RimStrength", float, 0.3)
    #property(RimWidth_, "RimWidth", float, 0.35)
    #property(Emissive_, "Emissive", Color, vec4(0.0, 0.0, 0.0, 1.0))
    #property(Opacity_, "Opacity", float, 1.0)

    CelSurface surface;
    surface.BaseColor = BaseColor_.rgb;
    surface.Normal = v_Normal;
    surface.WorldPosition = v_WorldPosition;
    surface.ShadowColor = ShadowColor_.rgb;
    surface.ShadeThreshold = ShadeThreshold_;
    surface.ShadeSoftness = ShadeSoftness_;
    surface.SpecularColor = SpecularColor_.rgb;
    surface.SpecularStrength = SpecularStrength_;
    surface.SpecularSmoothness = SpecularSmoothness_;
    surface.RimColor = RimColor_.rgb;
    surface.RimStrength = RimStrength_;
    surface.RimWidth = RimWidth_;

    outColor = vec4(ShadeCel(surface) + Emissive_.rgb, BaseColor_.a * Opacity_);

    uint id = u_ShaderData.NodeId;
    outNodeId = vec4(float(id & 0xFFu),
                     float((id >> 8) & 0xFFu),
                     float((id >> 16) & 0xFFu),
                     float((id >> 24) & 0xFFu)) / 255.0;
}

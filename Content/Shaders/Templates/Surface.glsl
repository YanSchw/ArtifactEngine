#shadergraph "Surface"

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
}

#type frag
#gen_buffers(frag)

void main() {
    #property(BaseColor_, "BaseColor", vec4, vec4(0.8, 0.8, 0.8, 1.0))
    #property(Emissive_, "Emissive", vec3, vec3(0.0))
    #property(Opacity_, "Opacity", float, 1.0)

    outColor = vec4(BaseColor_.rgb + Emissive_, BaseColor_.a * Opacity_);

    uint id = u_ShaderData.NodeId;
    outNodeId = vec4(float(id & 0xFFu),
                     float((id >> 8) & 0xFFu),
                     float((id >> 16) & 0xFFu),
                     float((id >> 24) & 0xFFu)) / 255.0;
}

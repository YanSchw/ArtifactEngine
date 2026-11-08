#blend Opaque
#depth Test
#cull None

#type vert
#include "/Shaders/Sky/SkyVertex.glsl"

#type frag
#include "/Shaders/Sky/Background.glsl"

void main() {
    float elevation = asin(clamp(ViewDirection().y, -1.0, 1.0)) / HalfPi;
    vec3 sky = elevation >= 0.0
        ? mix(u_GradientHorizon.rgb, u_GradientTop.rgb, elevation)
        : mix(u_GradientHorizon.rgb, u_GradientBottom.rgb, -elevation);
    WriteSky(sky);
}

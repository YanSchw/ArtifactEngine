#blend Opaque
#depth Test
#cull None

#type vert
#include "/Shaders/Sky/SkyVertex.glsl"

#type frag
#include "/Shaders/Sky/Background.glsl"

layout(binding = 16) uniform sampler2D u_Panorama;

void main() {
    vec3 direction = ViewDirection();
    float azimuth = atan(direction.x, direction.z) + u_SkyParams.y;
    vec2 uv = vec2(azimuth / TwoPi + 0.5, acos(clamp(direction.y, -1.0, 1.0)) / Pi);
    // Mip selection would see the seam where the azimuth wraps around and blur a line into it.
    WriteSky(textureLod(u_Panorama, uv, 0.0).rgb);
}

#blend Premultiplied
#depth Test
#cull None

#type vert
#include "/Shaders/Particles/ParticleVertex.glsl"

#type frag
#define PARTICLE_PREMULTIPLIED
#include "/Shaders/Particles/ParticleFragment.glsl"

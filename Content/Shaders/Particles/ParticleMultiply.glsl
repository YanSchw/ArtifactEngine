#blend Multiply
#depth Test
#cull None

#type vert
#include "/Shaders/Particles/ParticleVertex.glsl"

#type frag
#define PARTICLE_MULTIPLY
#include "/Shaders/Particles/ParticleFragment.glsl"

#pragma once

layout(binding = 1, std140) uniform SkyBlock {
    vec4 u_GradientTop;
    vec4 u_GradientHorizon;
    vec4 u_GradientBottom;
    vec4 u_SkyParams;
};

const float Pi = 3.14159265359;
const float TwoPi = 6.28318530718;
const float HalfPi = 1.57079632679;

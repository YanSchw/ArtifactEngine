#version 450
layout(location = 0) out vec4 outColor;

layout(location = 1) in vec4 v_Color;

void main() {
    outColor = vec4(v_Color.rgb, 1.0);
}
#pragma once

vec4 PackNodeId(uint InId) {
    return vec4(float(InId & 0xFFu),
                float((InId >> 8) & 0xFFu),
                float((InId >> 16) & 0xFFu),
                float((InId >> 24) & 0xFFu)) / 255.0;
}

vec3 EncodeNormal(vec3 InNormal) {
    return normalize(InNormal) * 0.5 + 0.5;
}

vec3 DecodeNormal(vec3 InEncoded) {
    return normalize(InEncoded * 2.0 - 1.0);
}

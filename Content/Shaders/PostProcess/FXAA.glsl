#blend Opaque
#depth None
#cull None

#type vert
layout(location = 0) in vec3 a_Position;

void main() {
    gl_Position = vec4(a_Position, 1.0);
}

#type frag
// Mirrors FxaaUniforms in AntiAliasingPass.cpp.
layout(binding = 1, std140) uniform FxaaBlock {
    // x: relative edge threshold, y: absolute edge threshold, z: sub-pixel blend
    vec4 u_FxaaParams;
};
layout(binding = 16) uniform sampler2D u_Source;

layout(location = 0) out vec4 outColor;

const int SearchSteps = 12;
const float SearchStepScale[SearchSteps] = float[](1.0, 1.0, 1.0, 1.0, 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0);

float Luma(vec3 InColor) {
    return dot(InColor, vec3(0.299, 0.587, 0.114));
}

// The chain is HDR; FXAA works on what the display shows.
vec3 SourceAt(vec2 InUV) {
    return clamp(texture(u_Source, InUV).rgb, 0.0, 1.0);
}

vec3 SourceOffset(vec2 InUV, ivec2 InOffset) {
    return SourceAt(InUV + vec2(InOffset) / vec2(textureSize(u_Source, 0)));
}

float LumaAt(vec2 InUV) {
    return Luma(SourceAt(InUV));
}

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(u_Source, 0));
    vec2 uv = gl_FragCoord.xy * texelSize;

    vec3 colorCenter = SourceAt(uv);
    float lumaCenter = Luma(colorCenter);
    float lumaDown = Luma(SourceOffset(uv, ivec2(0, -1)));
    float lumaUp = Luma(SourceOffset(uv, ivec2(0, 1)));
    float lumaLeft = Luma(SourceOffset(uv, ivec2(-1, 0)));
    float lumaRight = Luma(SourceOffset(uv, ivec2(1, 0)));

    float lumaMin = min(lumaCenter, min(min(lumaDown, lumaUp), min(lumaLeft, lumaRight)));
    float lumaMax = max(lumaCenter, max(max(lumaDown, lumaUp), max(lumaLeft, lumaRight)));
    float range = lumaMax - lumaMin;
    if (range < max(u_FxaaParams.y, lumaMax * u_FxaaParams.x)) {
        outColor = vec4(colorCenter, 1.0);
        return;
    }

    float lumaDownLeft = Luma(SourceOffset(uv, ivec2(-1, -1)));
    float lumaDownRight = Luma(SourceOffset(uv, ivec2(1, -1)));
    float lumaUpLeft = Luma(SourceOffset(uv, ivec2(-1, 1)));
    float lumaUpRight = Luma(SourceOffset(uv, ivec2(1, 1)));

    // How far the centre stands out from its neighbourhood decides the sub-pixel blur.
    float lumaAverage = (2.0 * (lumaDown + lumaUp + lumaLeft + lumaRight)
                       + lumaDownLeft + lumaDownRight + lumaUpLeft + lumaUpRight) / 12.0;
    float subpixel = smoothstep(0.0, 1.0, clamp(abs(lumaAverage - lumaCenter) / range, 0.0, 1.0));
    subpixel = subpixel * subpixel * u_FxaaParams.z;

    float horizontalEdge = abs(lumaDown + lumaUp - 2.0 * lumaCenter) * 2.0
                         + abs(lumaDownLeft + lumaUpLeft - 2.0 * lumaLeft)
                         + abs(lumaDownRight + lumaUpRight - 2.0 * lumaRight);
    float verticalEdge = abs(lumaLeft + lumaRight - 2.0 * lumaCenter) * 2.0
                       + abs(lumaDownLeft + lumaDownRight - 2.0 * lumaDown)
                       + abs(lumaUpLeft + lumaUpRight - 2.0 * lumaUp);
    bool horizontal = horizontalEdge >= verticalEdge;

    // Which side of the centre the edge lies on.
    float lumaNegative = horizontal ? lumaDown : lumaLeft;
    float lumaPositive = horizontal ? lumaUp : lumaRight;
    float gradientNegative = abs(lumaNegative - lumaCenter);
    float gradientPositive = abs(lumaPositive - lumaCenter);
    bool negativeSide = gradientNegative >= gradientPositive;
    float gradientScaled = 0.25 * max(gradientNegative, gradientPositive);
    float stepLength = (horizontal ? texelSize.y : texelSize.x) * (negativeSide ? -1.0 : 1.0);
    float lumaEdge = 0.5 * (lumaCenter + (negativeSide ? lumaNegative : lumaPositive));

    // Walk along the edge in both directions until its contrast ends.
    vec2 edgeUV = uv + (horizontal ? vec2(0.0, stepLength) : vec2(stepLength, 0.0)) * 0.5;
    vec2 edgeStep = horizontal ? vec2(texelSize.x, 0.0) : vec2(0.0, texelSize.y);
    vec2 uvBackward = edgeUV - edgeStep;
    vec2 uvForward = edgeUV + edgeStep;
    float deltaBackward = LumaAt(uvBackward) - lumaEdge;
    float deltaForward = LumaAt(uvForward) - lumaEdge;
    bool doneBackward = abs(deltaBackward) >= gradientScaled;
    bool doneForward = abs(deltaForward) >= gradientScaled;
    for (int i = 1; i < SearchSteps && !(doneBackward && doneForward); i++) {
        if (!doneBackward) {
            uvBackward -= edgeStep * SearchStepScale[i];
            deltaBackward = LumaAt(uvBackward) - lumaEdge;
            doneBackward = abs(deltaBackward) >= gradientScaled;
        }
        if (!doneForward) {
            uvForward += edgeStep * SearchStepScale[i];
            deltaForward = LumaAt(uvForward) - lumaEdge;
            doneForward = abs(deltaForward) >= gradientScaled;
        }
    }

    float distanceBackward = horizontal ? uv.x - uvBackward.x : uv.y - uvBackward.y;
    float distanceForward = horizontal ? uvForward.x - uv.x : uvForward.y - uv.y;
    bool backwardCloser = distanceBackward < distanceForward;
    float edgeOffset = 0.5 - min(distanceBackward, distanceForward) / (distanceBackward + distanceForward);

    // Only shift towards the edge when the nearer end varies the opposite way the centre does.
    bool centerDarker = lumaCenter < lumaEdge;
    bool endDarker = (backwardCloser ? deltaBackward : deltaForward) < 0.0;
    float offset = max(endDarker != centerDarker ? edgeOffset : 0.0, subpixel);

    vec2 finalUV = uv + (horizontal ? vec2(0.0, offset * stepLength) : vec2(offset * stepLength, 0.0));
    outColor = vec4(SourceAt(finalUV), 1.0);
}

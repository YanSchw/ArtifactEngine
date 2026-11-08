#pragma once
#include "Common/Types.h"
#include "Object/Enum.h"
#include "Object/Struct.h"
#include "PostProcessParams.gen.h"

ARTIFACT_ENUM();
enum class AntiAliasingMode : uint8_t {
    None = 0,
    FXAA = 1,
    TAA = 2
};

struct AntiAliasingParams {
    ARTIFACT_STRUCT();

    PROPERTY()
    AntiAliasingMode Mode = AntiAliasingMode::FXAA;

    /** Contrast an edge needs, relative to the brightest pixel around it. */
    PROPERTY()
    float FxaaEdgeThreshold = 0.166f;

    /** Absolute contrast below which dark areas are never treated as edges. */
    PROPERTY()
    float FxaaEdgeThresholdMin = 0.0833f;

    /** How much sub-pixel aliasing is smoothed away, 0 to 1. */
    PROPERTY()
    float FxaaSubpixelBlend = 0.75f;

    /** Share of the reprojected history kept each frame; higher is smoother but ghosts more. */
    PROPERTY()
    float TaaFeedback = 0.9f;
};

/** Everything the PostProcess stage is configured with. PostProcessVolumeNode blends these through
 *  reflection: float members (also inside vectors, colors and nested structs) are interpolated,
 *  everything else is taken from the most influential volume. */
struct PostProcessParams {
    ARTIFACT_STRUCT();

    PROPERTY()
    AntiAliasingParams AntiAliasing;

    /** Moves every blendable member of InOut InAlpha of the way towards InTarget. */
    static void Blend(PostProcessParams& InOut, const PostProcessParams& InTarget, float InAlpha);
};

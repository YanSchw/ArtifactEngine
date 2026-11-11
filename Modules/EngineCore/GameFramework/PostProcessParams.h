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
    bool Enabled = false;

    PROPERTY()
    AntiAliasingMode Mode = AntiAliasingMode::FXAA;

    /** Contrast an edge needs, relative to the brightest pixel around it. */
    PROPERTY()
    float FxaaEdgeThreshold = 0.166f;

    /** Absolute contrast below which dark areas are never treated as edges. */
    PROPERTY()
    float FxaaEdgeThresholdMin = 0.0833f;

    /** How much sub-pixel aliasing is smoothed away, 0 to 1. */
    PROPERTY(Range = (0, 1))
    float FxaaSubpixelBlend = 0.75f;

    /** Share of the reprojected history kept each frame; higher is smoother but ghosts more. */
    PROPERTY(Range = (0, 1))
    float TaaFeedback = 0.9f;
};

/** Screen-space ambient occlusion, darkening creases and contact points from the scene's depth. */
struct AmbientOcclusionParams {
    ARTIFACT_STRUCT();

    PROPERTY()
    bool Enabled = false;

    /** How strongly occlusion darkens the image; 0 turns the effect off. */
    PROPERTY()
    float Intensity = 1.0f;

    /** World-space distance around a point that is searched for occluders. */
    PROPERTY()
    float Radius = 0.5f;

    /** World-space offset that keeps flat and gently curved surfaces from occluding themselves. */
    PROPERTY()
    float Bias = 0.025f;

    /** Exponent applied to the occlusion; above 1 deepens creases and lightens open areas. */
    PROPERTY()
    float Power = 1.5f;

    /** Colour fully occluded areas are pulled towards. */
    PROPERTY()
    Color Tint = Color(0.0f, 0.0f, 0.0f, 1.0f);

    /** Brightly lit pixels receive this much less occlusion, 0 to 1, as the occlusion is applied
     *  after lighting and would otherwise darken direct light too. */
    PROPERTY(Range = (0, 1))
    float LuminanceInfluence = 0.0f;

    /** Camera distance at which occlusion has faded out completely. */
    PROPERTY()
    float MaxDistance = 100.0f;

    /** Share of MaxDistance over which occlusion fades out, 0 to 1. */
    PROPERTY(Range = (0, 1))
    float DistanceFalloff = 0.25f;

    /** Occluders searched per pixel, 4 to 32. */
    PROPERTY(Range = (4, 32))
    int32_t SampleCount = 12;

    /** Computes occlusion at half the resolution in each direction, roughly four times cheaper. */
    PROPERTY()
    bool HalfResolution = true;

    /** Smooths the sampling noise away while keeping depth edges sharp. */
    PROPERTY()
    bool Blur = true;

    /** How strongly the blur respects depth edges; higher keeps edges crisper but leaves more noise. */
    PROPERTY()
    float BlurSharpness = 8.0f;
};

/** Light bleeding out of the brightest parts of the image. */
struct BloomParams {
    ARTIFACT_STRUCT();

    PROPERTY()
    bool Enabled = false;

    /** How much bloom is added onto the image; 0 turns the effect off. */
    PROPERTY()
    float Intensity = 0.5f;

    /** Brightness at which pixels start to bloom. The scene is HDR, so 1 is the brightest a display shows. */
    PROPERTY()
    float Threshold = 1.0f;

    /** Width of the transition around Threshold, 0 (hard cut) to 1 (very gradual). */
    PROPERTY(Range = (0, 1))
    float SoftKnee = 0.5f;

    /** How far the bloom spreads out, 0 to 1. */
    PROPERTY(Range = (0, 1))
    float Scatter = 0.7f;

    /** Brightness every pixel is clamped to before it blooms, so single hot pixels do not flicker. */
    PROPERTY()
    float MaxBrightness = 20.0f;

    PROPERTY()
    Color Tint = Color(1.0f, 1.0f, 1.0f, 1.0f);

    /** How many times the image is halved for the blur, 1 to 8; more spreads wider and costs more. */
    PROPERTY(Range = (1, 8))
    int32_t MipCount = 6;
};

/** Colour fringes towards the screen's edges, as from a cheap lens. */
struct ChromaticAberrationParams {
    ARTIFACT_STRUCT();

    PROPERTY()
    bool Enabled = false;

    /** Strength of the fringes; 1 splits the outermost colours by 1% of the screen at the corners,
     *  0 turns the effect off. */
    PROPERTY()
    float Intensity = 0.5f;

    /** Distance from the centre where the fringes begin, 0 (centre) to 1 (corners). */
    PROPERTY(Range = (0, 1))
    float StartRadius = 0.2f;

    /** Shapes the growth from StartRadius to the corners; above 1 keeps the fringes to the edges. */
    PROPERTY()
    float Falloff = 1.5f;

    /** Taps across the spectrum, 3 to 16. 3 is a plain red/green/blue split, more blend into a rainbow. */
    PROPERTY(Range = (3, 16))
    int32_t SampleCount = 6;
};

/** Darkening towards the screen's edges. */
struct VignetteParams {
    ARTIFACT_STRUCT();

    PROPERTY()
    bool Enabled = false;

    /** How far the darkening reaches in; 0 turns the effect off. */
    PROPERTY()
    float Intensity = 0.1f;

    /** Softness of the falloff, 0.01 (hard) to 1 (soft). */
    PROPERTY(Range = (0.01, 1))
    float Smoothness = 0.4f;

    /** 1 is a perfect circle, 0 follows the screen's aspect ratio. */
    PROPERTY(Range = (0, 1))
    float Roundness = 0.0f;

    /** Centre of the vignette in screen space, (0.5, 0.5) being the middle of the screen. */
    PROPERTY()
    Vec2 Center = Vec2(0.5f);

    /** Colour the edges are pulled towards. */
    PROPERTY()
    Color Tint = Color(0.0f, 0.0f, 0.0f, 1.0f);
};

/** Photographic film noise laid over the final image. */
struct FilmGrainParams {
    ARTIFACT_STRUCT();

    PROPERTY()
    bool Enabled = false;

    /** How visible the grain is; 0 turns the effect off. */
    PROPERTY()
    float Intensity = 0.1f;

    /** Size of a grain in pixels at 1080p; it scales with the resolution. */
    PROPERTY()
    float Size = 1.6f;

    /** How much the grain fades out in bright areas, 0 (even everywhere) to 1 (only in shadows). */
    PROPERTY(Range = (0, 1))
    float LuminanceResponse = 0.8f;

    /** 0 is monochrome grain, 1 gives each colour channel its own grain. */
    PROPERTY(Range = (0, 1))
    float Colorfulness = 0.0f;

    /** New grain this many times per second; 0 renews it every frame. */
    PROPERTY()
    float FramesPerSecond = 0.0f;

    /** Animates the grain; when off, the same pattern stays on screen. */
    PROPERTY()
    bool Animated = true;
};

ARTIFACT_ENUM();
enum class TonemappingMode : uint8_t {
    None = 0,
    Linear = 1,
    Reinhard = 2,
    Hable = 3,
    ACES = 4,
    AgX = 5,
    Neutral = 6
};

struct TonemappingParams {
    ARTIFACT_STRUCT();

    PROPERTY()
    bool Enabled = false;

    PROPERTY()
    TonemappingMode Mode = TonemappingMode::ACES;

    PROPERTY()
    float Exposure = 0.0f;

    PROPERTY()
    float WhitePoint = 4.0f;
};

/** Everything the PostProcess stage is configured with. */
struct PostProcessParams {
    ARTIFACT_STRUCT();

    PROPERTY()
    AmbientOcclusionParams AmbientOcclusion;

    PROPERTY()
    BloomParams Bloom;

    PROPERTY()
    TonemappingParams Tonemapping;

    PROPERTY()
    AntiAliasingParams AntiAliasing;

    PROPERTY()
    ChromaticAberrationParams ChromaticAberration;

    PROPERTY()
    VignetteParams Vignette;

    PROPERTY()
    FilmGrainParams FilmGrain;

    /** What the stage runs with where no volume enables an effect: every effect at zero intensity. */
    static PostProcessParams Neutral();

    /** The effect structs, each gated by its Enabled member. */
    static const Array<StructProperty*>& GetEffects();
    static bool IsEffectEnabled(const PostProcessParams& InParams, const StructProperty& InEffect);
    static void SetEffectEnabled(PostProcessParams& InOut, const StructProperty& InEffect, bool InEnabled);
    /** Moves every blendable member of one effect of InOut InAlpha of the way towards InTarget. */
    static void BlendEffect(PostProcessParams& InOut, const PostProcessParams& InTarget, const StructProperty& InEffect, float InAlpha);
    static void CopyEffect(PostProcessParams& OutParams, const PostProcessParams& InSource, const StructProperty& InEffect);
};

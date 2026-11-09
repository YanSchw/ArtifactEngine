#pragma once
#include "Common/Array.h"
#include "Common/Types.h"
#include "Object/Enum.h"
#include "Object/Pointer.h"
#include "Object/Struct.h"
#include "ParticleModules.gen.h"

class Material;
class Mesh;
class Texture2D;

ARTIFACT_ENUM();
enum class ParticleSimulationSpace : uint8_t {
    Local = 0,
    World = 1
};

ARTIFACT_ENUM();
enum class ParticleCurveInterpolation : uint8_t {
    Linear = 0,
    Smooth = 1
};

ARTIFACT_ENUM();
enum class ParticleShapeType : uint8_t {
    Point = 0,
    Sphere = 1,
    Hemisphere = 2,
    Cone = 3,
    Box = 4,
    Circle = 5,
    Edge = 6
};

ARTIFACT_ENUM();
enum class ParticleRenderMode : uint8_t {
    Billboard = 0,
    StretchedBillboard = 1,
    HorizontalBillboard = 2,
    VerticalBillboard = 3,
    Mesh = 4,
    None = 5
};

ARTIFACT_ENUM();
enum class ParticleBlendMode : uint8_t {
    Alpha = 0,
    Additive = 1,
    Premultiplied = 2,
    Multiply = 3
};

ARTIFACT_ENUM();
enum class ParticleSortMode : uint8_t {
    None = 0,
    ByDistance = 1,
    OldestInFront = 2,
    YoungestInFront = 3
};

ARTIFACT_ENUM();
enum class ParticleCollisionType : uint8_t {
    Planes = 0,
    World = 1
};

ARTIFACT_ENUM();
enum class ParticleSubEmitterTrigger : uint8_t {
    Birth = 0,
    Alive = 1,
    Death = 2,
    Collision = 3
};

ARTIFACT_ENUM();
enum class ParticleTrailMode : uint8_t {
    PerParticle = 0,
    Ribbon = 1
};

struct ParticleFloatRange {
    ARTIFACT_STRUCT();

    ParticleFloatRange() = default;
    ParticleFloatRange(float InValue) : Min(InValue), Max(InValue) {}
    ParticleFloatRange(float InMin, float InMax) : Min(InMin), Max(InMax) {}

    float Evaluate(float InAlpha) const { return Min + (Max - Min) * InAlpha; }

    PROPERTY()
    float Min = 1.0f;

    PROPERTY()
    float Max = 1.0f;
};

struct ParticleColorRange {
    ARTIFACT_STRUCT();

    Color Evaluate(float InAlpha) const { return Min + (Max - Min) * InAlpha; }

    PROPERTY()
    Color Min = Color(1.0f);

    PROPERTY()
    Color Max = Color(1.0f);
};

struct ParticleVectorRange {
    ARTIFACT_STRUCT();

    ParticleVectorRange() = default;
    ParticleVectorRange(const Vec3& InValue) : Min(InValue), Max(InValue) {}

    Vec3 Evaluate(const Vec3& InAlpha) const { return Min + (Max - Min) * InAlpha; }

    PROPERTY()
    Vec3 Min = Vec3(0.0f);

    PROPERTY()
    Vec3 Max = Vec3(0.0f);
};

struct ParticleCurveKey {
    ARTIFACT_STRUCT();

    ParticleCurveKey() = default;
    ParticleCurveKey(float InTime, float InValue) : Time(InTime), Value(InValue) {}

    PROPERTY()
    float Time = 0.0f;

    PROPERTY()
    float Value = 1.0f;
};

/** Evaluates to 1 without keys. */
struct ParticleCurve {
    ARTIFACT_STRUCT();

    ParticleCurve() = default;
    ParticleCurve(const Array<ParticleCurveKey>& InKeys) : Keys(InKeys) {}

    float Evaluate(float InTime) const;

    PROPERTY()
    Array<ParticleCurveKey> Keys;

    PROPERTY()
    ParticleCurveInterpolation Interpolation = ParticleCurveInterpolation::Linear;
};

struct ParticleGradientKey {
    ARTIFACT_STRUCT();

    ParticleGradientKey() = default;
    ParticleGradientKey(float InTime, const Color& InValue) : Time(InTime), Value(InValue) {}

    PROPERTY()
    float Time = 0.0f;

    PROPERTY()
    Color Value = Color(1.0f);
};

/** Evaluates to white without keys. */
struct ParticleGradient {
    ARTIFACT_STRUCT();

    ParticleGradient() = default;
    ParticleGradient(const Array<ParticleGradientKey>& InKeys) : Keys(InKeys) {}

    Color Evaluate(float InTime) const;

    PROPERTY()
    Array<ParticleGradientKey> Keys;

    PROPERTY()
    ParticleCurveInterpolation Interpolation = ParticleCurveInterpolation::Linear;
};

struct ParticleMainModule {
    ARTIFACT_STRUCT();

    PROPERTY()
    float Duration = 5.0f;

    PROPERTY()
    bool Looping = true;

    PROPERTY()
    bool Prewarm = false;

    PROPERTY()
    ParticleFloatRange StartDelay = ParticleFloatRange(0.0f);

    PROPERTY()
    ParticleFloatRange StartLifetime = ParticleFloatRange(5.0f);

    PROPERTY()
    ParticleFloatRange StartSpeed = ParticleFloatRange(5.0f);

    PROPERTY()
    ParticleFloatRange StartSize = ParticleFloatRange(1.0f);

    PROPERTY()
    Vec3 StartSizeAxes = Vec3(1.0f);

    /** Degrees. Billboards roll around Z. */
    PROPERTY()
    ParticleVectorRange StartRotation;

    PROPERTY()
    ParticleColorRange StartColor;

    PROPERTY()
    float GravityModifier = 0.0f;

    PROPERTY()
    ParticleSimulationSpace SimulationSpace = ParticleSimulationSpace::Local;

    PROPERTY()
    float SimulationSpeed = 1.0f;

    PROPERTY()
    int32_t MaxParticles = 1000;

    PROPERTY()
    bool AutoRandomSeed = true;

    PROPERTY()
    uint32_t RandomSeed = 0;
};

struct ParticleBurst {
    ARTIFACT_STRUCT();

    PROPERTY()
    bool Enabled = true;

    PROPERTY()
    float Time = 0.0f;

    PROPERTY()
    ParticleFloatRange Count = ParticleFloatRange(30.0f);

    PROPERTY()
    int32_t Cycles = 1;

    PROPERTY()
    float Interval = 0.01f;

    PROPERTY()
    float Probability = 1.0f;
};

struct ParticleEmissionModule {
    ARTIFACT_STRUCT();

    PROPERTY()
    bool Enabled = true;

    PROPERTY()
    float RateOverTime = 10.0f;

    PROPERTY()
    ParticleCurve RateOverTimeCurve;

    PROPERTY()
    float RateOverDistance = 0.0f;

    PROPERTY()
    Array<ParticleBurst> Bursts;
};

struct ParticleShapeModule {
    ARTIFACT_STRUCT();

    PROPERTY()
    bool Enabled = true;

    PROPERTY()
    ParticleShapeType Shape = ParticleShapeType::Cone;

    PROPERTY()
    float Radius = 1.0f;

    PROPERTY()
    float RadiusThickness = 1.0f;

    PROPERTY()
    float Angle = 25.0f;

    PROPERTY()
    float Arc = 360.0f;

    PROPERTY()
    Vec3 BoxSize = Vec3(1.0f);

    PROPERTY()
    Vec3 Position = Vec3(0.0f);

    PROPERTY()
    Vec3 Rotation = Vec3(0.0f);

    PROPERTY()
    float RandomizeDirection = 0.0f;
};

struct ParticleVelocityOverLifetimeModule {
    ARTIFACT_STRUCT();

    PROPERTY()
    bool Enabled = false;

    PROPERTY()
    Vec3 Linear = Vec3(0.0f);

    PROPERTY()
    ParticleSimulationSpace Space = ParticleSimulationSpace::Local;

    PROPERTY()
    Vec3 Orbital = Vec3(0.0f);

    PROPERTY()
    float Radial = 0.0f;

    PROPERTY()
    ParticleCurve SpeedModifier;
};

struct ParticleInheritVelocityModule {
    ARTIFACT_STRUCT();

    PROPERTY()
    bool Enabled = false;

    PROPERTY()
    float Multiplier = 1.0f;
};

struct ParticleLimitVelocityOverLifetimeModule {
    ARTIFACT_STRUCT();

    PROPERTY()
    bool Enabled = false;

    PROPERTY()
    float Speed = 1.0f;

    PROPERTY()
    ParticleCurve SpeedOverLifetime;

    /** How fast speed above the limit is removed, per second. */
    PROPERTY()
    float Dampen = 10.0f;
};

struct ParticleForceOverLifetimeModule {
    ARTIFACT_STRUCT();

    PROPERTY()
    bool Enabled = false;

    PROPERTY()
    Vec3 Force = Vec3(0.0f);

    PROPERTY()
    ParticleSimulationSpace Space = ParticleSimulationSpace::Local;

    PROPERTY()
    float Drag = 0.0f;
};

struct ParticleNoiseModule {
    ARTIFACT_STRUCT();

    PROPERTY()
    bool Enabled = false;

    PROPERTY()
    float Strength = 1.0f;

    PROPERTY()
    float Frequency = 0.5f;

    PROPERTY()
    float ScrollSpeed = 0.0f;

    PROPERTY()
    int32_t Octaves = 1;
};

/** Relative to the system's transform. */
struct ParticleCollisionPlane {
    ARTIFACT_STRUCT();

    PROPERTY()
    Vec3 Position = Vec3(0.0f);

    PROPERTY()
    Vec3 Normal = Vec3(0.0f, 1.0f, 0.0f);
};

struct ParticleCollisionModule {
    ARTIFACT_STRUCT();

    PROPERTY()
    bool Enabled = false;

    PROPERTY()
    ParticleCollisionType Type = ParticleCollisionType::Planes;

    PROPERTY()
    Array<ParticleCollisionPlane> Planes;

    PROPERTY()
    float Bounce = 0.5f;

    PROPERTY()
    float Dampen = 0.0f;

    PROPERTY()
    float LifetimeLoss = 0.0f;

    PROPERTY()
    float RadiusScale = 1.0f;

    PROPERTY()
    float MinKillSpeed = 0.0f;
};

struct ParticleColorOverLifetimeModule {
    ARTIFACT_STRUCT();

    PROPERTY()
    bool Enabled = false;

    PROPERTY()
    ParticleGradient Gradient = ParticleGradient({ { 0.0f, Color(1.0f) }, { 1.0f, Color(1.0f, 1.0f, 1.0f, 0.0f) } });
};

struct ParticleColorBySpeedModule {
    ARTIFACT_STRUCT();

    PROPERTY()
    bool Enabled = false;

    PROPERTY()
    ParticleGradient Gradient;

    PROPERTY()
    ParticleFloatRange SpeedRange = ParticleFloatRange(0.0f, 1.0f);
};

struct ParticleSizeOverLifetimeModule {
    ARTIFACT_STRUCT();

    PROPERTY()
    bool Enabled = false;

    PROPERTY()
    ParticleCurve Size = ParticleCurve({ { 0.0f, 1.0f }, { 1.0f, 0.0f } });
};

struct ParticleSizeBySpeedModule {
    ARTIFACT_STRUCT();

    PROPERTY()
    bool Enabled = false;

    PROPERTY()
    ParticleCurve Size;

    PROPERTY()
    ParticleFloatRange SpeedRange = ParticleFloatRange(0.0f, 1.0f);
};

struct ParticleRotationOverLifetimeModule {
    ARTIFACT_STRUCT();

    PROPERTY()
    bool Enabled = false;

    PROPERTY()
    ParticleVectorRange AngularVelocity = ParticleVectorRange(Vec3(0.0f, 0.0f, 45.0f));
};

struct ParticleRotationBySpeedModule {
    ARTIFACT_STRUCT();

    PROPERTY()
    bool Enabled = false;

    /** Degrees per second, reached at the top of the speed range. */
    PROPERTY()
    Vec3 AngularVelocity = Vec3(0.0f, 0.0f, 45.0f);

    PROPERTY()
    ParticleFloatRange SpeedRange = ParticleFloatRange(0.0f, 1.0f);
};

struct ParticleSubEmitter {
    ARTIFACT_STRUCT();

    PROPERTY()
    bool Enabled = true;

    PROPERTY()
    ParticleSubEmitterTrigger Trigger = ParticleSubEmitterTrigger::Death;

    /** Index into the system's emitters. */
    PROPERTY()
    int32_t EmitterIndex = 1;

    /** Per trigger, or per second for Alive. */
    PROPERTY()
    ParticleFloatRange Count = ParticleFloatRange(10.0f);

    PROPERTY()
    float Probability = 1.0f;

    PROPERTY()
    bool InheritColor = false;

    PROPERTY()
    bool InheritSize = false;

    PROPERTY()
    float InheritVelocity = 0.0f;
};

struct ParticleTextureSheetModule {
    ARTIFACT_STRUCT();

    PROPERTY()
    bool Enabled = false;

    PROPERTY()
    int32_t TilesX = 1;

    PROPERTY()
    int32_t TilesY = 1;

    /** 0 uses every tile. */
    PROPERTY()
    int32_t FrameCount = 0;

    PROPERTY()
    float Cycles = 1.0f;

    /** Above 0 the sheet plays at this rate instead of Cycles over the particle's lifetime. */
    PROPERTY()
    float FramesPerSecond = 0.0f;

    PROPERTY()
    bool RandomStartFrame = false;

    PROPERTY()
    bool FrameBlending = false;
};

struct ParticleTrailModule {
    ARTIFACT_STRUCT();

    PROPERTY()
    bool Enabled = false;

    PROPERTY()
    ParticleTrailMode Mode = ParticleTrailMode::PerParticle;

    PROPERTY()
    float Lifetime = 0.5f;

    PROPERTY()
    float MinVertexDistance = 0.1f;

    /** Relative to the particle size. */
    PROPERTY()
    float Width = 1.0f;

    /** From head to tail. */
    PROPERTY()
    ParticleCurve WidthOverTrail;

    /** From head to tail. */
    PROPERTY()
    ParticleGradient ColorOverTrail;

    PROPERTY()
    bool InheritParticleColor = true;

    PROPERTY()
    WeakObjectPtr<Texture2D> Texture;
};

struct ParticleRendererModule {
    ARTIFACT_STRUCT();

    PROPERTY()
    ParticleRenderMode RenderMode = ParticleRenderMode::Billboard;

    PROPERTY()
    ParticleBlendMode BlendMode = ParticleBlendMode::Alpha;

    PROPERTY()
    WeakObjectPtr<Texture2D> Texture;

    /** Replaces the built-in particle shader. Must render Forward. */
    PROPERTY()
    WeakObjectPtr<Material> CustomMaterial;

    PROPERTY()
    WeakObjectPtr<Mesh> RenderMesh;

    PROPERTY()
    float Brightness = 1.0f;

    PROPERTY()
    bool Lit = false;

    PROPERTY()
    ParticleSortMode SortMode = ParticleSortMode::ByDistance;

    PROPERTY()
    float LengthScale = 2.0f;

    PROPERTY()
    float SpeedScale = 0.0f;

    PROPERTY()
    bool AlignToVelocity = false;

    PROPERTY()
    float SoftParticleDistance = 0.0f;

    /** Fades particles in between these camera distances. */
    PROPERTY()
    float CameraFadeNear = 0.0f;

    PROPERTY()
    float CameraFadeFar = 0.0f;
};

struct ParticleEmitter {
    ARTIFACT_STRUCT();

    PROPERTY()
    String Name = "Emitter";

    PROPERTY()
    bool Enabled = true;

    PROPERTY()
    ParticleMainModule Main;

    PROPERTY()
    ParticleEmissionModule Emission;

    PROPERTY()
    ParticleShapeModule Shape;

    PROPERTY()
    ParticleVelocityOverLifetimeModule VelocityOverLifetime;

    PROPERTY()
    ParticleInheritVelocityModule InheritVelocity;

    PROPERTY()
    ParticleLimitVelocityOverLifetimeModule LimitVelocityOverLifetime;

    PROPERTY()
    ParticleForceOverLifetimeModule ForceOverLifetime;

    PROPERTY()
    ParticleNoiseModule Noise;

    PROPERTY()
    ParticleCollisionModule Collision;

    PROPERTY()
    ParticleColorOverLifetimeModule ColorOverLifetime;

    PROPERTY()
    ParticleColorBySpeedModule ColorBySpeed;

    PROPERTY()
    ParticleSizeOverLifetimeModule SizeOverLifetime;

    PROPERTY()
    ParticleSizeBySpeedModule SizeBySpeed;

    PROPERTY()
    ParticleRotationOverLifetimeModule RotationOverLifetime;

    PROPERTY()
    ParticleRotationBySpeedModule RotationBySpeed;

    PROPERTY()
    Array<ParticleSubEmitter> SubEmitters;

    PROPERTY()
    ParticleTextureSheetModule TextureSheetAnimation;

    PROPERTY()
    ParticleTrailModule Trails;

    PROPERTY()
    ParticleRendererModule Renderer;
};

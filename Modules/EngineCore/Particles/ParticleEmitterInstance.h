#pragma once
#include "Common/Array.h"
#include "Common/Types.h"
#include "Particles/ParticleHandle.h"
#include "Particles/ParticleModules.h"

struct Particle {
    Vec3 Position = Vec3(0.0f);
    Vec3 Velocity = Vec3(0.0f);
    Vec3 CurrentVelocity = Vec3(0.0f);
    Color StartColor = Color(1.0f);
    Color CurrentColor = Color(1.0f);
    Vec3 Rotation = Vec3(0.0f);
    Vec3 AngularVelocity = Vec3(0.0f);
    float Age = 0.0f;
    float Lifetime = 1.0f;
    float StartSize = 1.0f;
    float CurrentSize = 1.0f;
    float Random = 0.0f;
    /** xyz is the position, w the particle's age when it was recorded. */
    Array<Vec4> Trail;
};

struct ParticleSubEmit {
    ParticleEmitParams Params;
    int32_t Count = 0;
};

using ParticleRayCast = std::function<bool(const Vec3& InFrom, const Vec3& InTo, Vec3& OutPosition, Vec3& OutNormal)>;

class ParticleEmitterInstance {
public:
    void Play(const ParticleEmitter& InEmitter, uint32_t InSeed, bool InEmitting, const Mat4& InTransform);
    void Stop() { m_Emitting = false; }
    void Clear() { m_Particles.Clear(); }
    void Emit(const ParticleEmitter& InEmitter, const Mat4& InTransform, int32_t InCount, const ParticleEmitParams& InParams);
    void Step(const ParticleEmitter& InEmitter, float InDeltaTime, const Mat4& InFrom, const Mat4& InTo, const ParticleRayCast& InRayCast);

    bool IsAlive() const { return m_Emitting || !m_Particles.IsEmpty(); }
    bool IsEmitting() const { return m_Emitting; }

    Array<Particle>& GetParticles() { return m_Particles; }
    const Array<Particle>& GetParticles() const { return m_Particles; }
    Array<ParticleSubEmit>& GetSubEmits() { return m_SubEmits; }

    ParticleSimulationSpace GetSimulationSpace() const { return m_Space; }
    Vec3 ToWorldPosition(const Vec3& InPosition, const Mat4& InTransform) const;
    Vec3 ToWorldDirection(const Vec3& InDirection, const Mat4& InTransform) const;

    static Mat4 BlendTranslation(const Mat4& InFrom, const Mat4& InTo, float InAlpha);
    static float AverageScale(const Mat3& InBasis);

private:
    struct EmitStep;

    void UpdateParticles(const ParticleEmitter& InEmitter, float InDeltaTime, const Mat4& InTransform, const ParticleRayCast& InRayCast);
    bool Collide(const ParticleCollisionModule& InCollision, Particle& OutParticle, const Vec3& InPrevious,
                 const Mat4& InTransform, const ParticleRayCast& InRayCast);
    void Kill(const ParticleEmitter& InEmitter, int32_t InIndex, const Mat4& InTransform);
    void TriggerSubEmitters(const ParticleEmitter& InEmitter, const Particle& InParticle, ParticleSubEmitterTrigger InTrigger,
                            const Mat4& InTransform, float InDeltaTime = 0.0f);
    void EmitOverStep(const ParticleEmitter& InEmitter, const EmitStep& InStep);
    void EmitWindow(const ParticleEmitter& InEmitter, const EmitStep& InStep, float InFrom, float InTo, float InCycleOffset);
    void FireBurst(const ParticleEmitter& InEmitter, const EmitStep& InStep, const ParticleBurst& InBurst,
                   int32_t& InOutCyclesFired, float InFrom, float InTo, float InCycleOffset);
    void SpawnSpread(const ParticleEmitter& InEmitter, const EmitStep& InStep, int32_t InCount, float InFromAlpha, float InToAlpha);
    bool Spawn(const ParticleEmitter& InEmitter, const Mat4& InTransform, float InPreAdvance, const ParticleEmitParams& InParams);
    void SampleShape(const ParticleShapeModule& InShape, Vec3& OutPosition, Vec3& OutDirection);
    void RefreshAppearance(const ParticleEmitter& InEmitter, Particle& OutParticle) const;
    void Prewarm(const ParticleEmitter& InEmitter, const Mat4& InTransform);
    void ApplySpace(const ParticleEmitter& InEmitter);

    uint32_t NextRandom();
    float Random01();
    Vec3 RandomVector();
    Vec3 RandomUnitVector();

    Array<Particle> m_Particles;
    Array<ParticleSubEmit> m_SubEmits;
    Array<ParticleCollisionPlane> m_Planes;
    ParticleSimulationSpace m_Space = ParticleSimulationSpace::Local;

    float m_Time = 0.0f;
    float m_TotalTime = 0.0f;
    float m_RateRemainder = 0.0f;
    float m_DistanceRemainder = 0.0f;
    Array<int32_t> m_BurstCyclesFired;
    Vec3 m_EmitterVelocity = Vec3(0.0f);
    uint32_t m_RandomState = 1;
    bool m_Emitting = false;
};

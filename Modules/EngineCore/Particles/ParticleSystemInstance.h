#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "Assets/Asset.h"
#include "Common/Array.h"
#include "Common/Types.h"
#include "Particles/ParticleEmitterInstance.h"
#include "Particles/ParticleHandle.h"
#include "ParticleSystemInstance.gen.h"

class ParticleSystemAsset;

class ParticleSystemInstance : public Object {
public:
    ARTIFACT_CLASS();

    ParticleSystemAsset* GetSystem() const;
    void SetSystem(ParticleSystemAsset* InSystem);

    void Play();
    void Stop(ParticleStopBehavior InBehavior = ParticleStopBehavior::StopEmitting);
    void SetPaused(bool InPaused);
    void Clear();
    void Restart();
    void Emit(int32_t InCount, const ParticleEmitParams& InParams = {});
    /** Replays from the start with the same seed up to InTime. */
    void Seek(float InTime);

    void Simulate(float InDeltaTime, const ParticleRayCast& InRayCast = {});

    bool IsAlive() const;
    bool IsPlaying() const { return IsAlive() && !m_Paused; }
    bool IsEmitting() const;
    bool IsPaused() const { return m_Paused; }

    const Mat4& GetTransform() const { return m_Transform; }
    void SetTransform(const Mat4& InTransform) { m_Transform = InTransform; }

    float GetSimulationSpeed() const { return m_SimulationSpeed; }
    void SetSimulationSpeed(float InSpeed) { m_SimulationSpeed = glm::max(InSpeed, 0.0f); }

    bool GetReleaseWhenFinished() const { return m_ReleaseWhenFinished; }
    void SetReleaseWhenFinished(bool InRelease) { m_ReleaseWhenFinished = InRelease; }

    float GetPlaybackTime() const { return m_PlaybackTime; }
    int32_t GetParticleCount() const;

    int32_t GetEmitterCount() const { return m_Emitters.Size(); }
    ParticleEmitterInstance& GetEmitter(int32_t InIndex) { return m_Emitters[InIndex]; }
    const ParticleEmitterInstance& GetEmitter(int32_t InIndex) const { return m_Emitters[InIndex]; }
    Array<Particle>& GetParticles(int32_t InEmitter = 0) { return m_Emitters[InEmitter].GetParticles(); }

    std::function<void()> OnFinished;

private:
    void PlayEmitters(const ParticleSystemAsset& InSystem, uint32_t InSeed);
    void SyncEmitters(const ParticleSystemAsset& InSystem);
    void Step(const ParticleSystemAsset& InSystem, float InDeltaTime, const Mat4& InFrom, const Mat4& InTo, const ParticleRayCast& InRayCast);
    void ProcessSubEmits(const ParticleSystemAsset& InSystem, const Mat4& InTransform);
    bool IsEmitterActive(int32_t InIndex) const;

    WeakObjectPtr<ParticleSystemAsset> m_System;
    AssetStreamHandle m_SystemHandle;
    Array<ParticleEmitterInstance> m_Emitters;

    Mat4 m_Transform = Mat4(1.0f);
    Mat4 m_PreviousTransform = Mat4(1.0f);

    float m_PlaybackTime = 0.0f;
    float m_SimulationSpeed = 1.0f;
    uint32_t m_Seed = 0;

    bool m_Paused = false;
    bool m_WasAlive = false;
    bool m_ReleaseWhenFinished = false;
    bool m_HasTransformHistory = false;
};

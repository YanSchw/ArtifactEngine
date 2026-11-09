#pragma once
#include "Node3D.h"
#include "Object/Enum.h"
#include "Object/Pointer.h"
#include "Particles/ParticleHandle.h"
#include "ParticleSystemEmitterNode.gen.h"

class ParticleSystemAsset;
class ParticleWorld;

ARTIFACT_ENUM();
enum class ParticleStopAction : uint8_t {
    None = 0,
    Destroy = 1
};

class ParticleSystemEmitterNode : public Node3D {
public:
    ARTIFACT_CLASS();

    virtual void BeginPlay() override;
    virtual void EndPlay() override;
    virtual void WorldUpdate(float InDeltaTime) override;

    void Play();
    void Stop(ParticleStopBehavior InBehavior = ParticleStopBehavior::StopEmitting);
    void Pause() { SetPaused(true); }
    void SetPaused(bool InPaused);
    void Clear();
    void Restart();
    void Emit(int32_t InCount, const ParticleEmitParams& InParams = {});

    bool IsPlaying() const { return m_Instance.IsPlaying(); }
    bool IsPaused() const { return m_Instance.IsPaused(); }
    bool IsEmitting() const { return m_Instance.IsEmitting(); }
    bool IsAlive() const { return m_Instance.IsAlive(); }
    int32_t GetParticleCount() const { return m_Instance.GetParticleCount(); }
    const ParticleHandle& GetInstance() const { return m_Instance; }

    ParticleSystemAsset* GetSystem() const;
    void SetSystem(ParticleSystemAsset* InSystem);

    bool GetAutoPlay() const { return m_AutoPlay; }
    void SetAutoPlay(bool InAutoPlay) { m_AutoPlay = InAutoPlay; }
    float GetSimulationSpeed() const { return m_SimulationSpeed; }
    void SetSimulationSpeed(float InSpeed);
    ParticleStopAction GetStopAction() const { return m_StopAction; }
    void SetStopAction(ParticleStopAction InAction) { m_StopAction = InAction; }
    bool GetDetachOnDestroy() const { return m_DetachOnDestroy; }
    void SetDetachOnDestroy(bool InDetach) { m_DetachOnDestroy = InDetach; }

    std::function<void()> OnFinished;

protected:
    virtual void UnInitializeNode(World& OutWorld) override;

private:
    void DestroyInstance(World& InWorld);
    void DetachInstance();
    ParticleWorld* GetParticleWorld() const;
    ParticleSystemInstance* EnsureInstance();

    PROPERTY()
    WeakObjectPtr<ParticleSystemAsset> m_System;

    PROPERTY()
    bool m_AutoPlay = true;

    PROPERTY()
    float m_SimulationSpeed = 1.0f;

    PROPERTY()
    ParticleStopAction m_StopAction = ParticleStopAction::None;

    /** Stops emitting and lets the remaining particles die out when the node ends play. */
    PROPERTY()
    bool m_DetachOnDestroy = true;

    ParticleHandle m_Instance;
    bool m_WasAlive = false;
};

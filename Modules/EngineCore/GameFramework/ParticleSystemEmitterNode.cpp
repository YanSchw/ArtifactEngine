#include "ParticleSystemEmitterNode.h"
#include "ParticleWorld.h"
#include "World.h"
#include "Assets/ParticleSystemAsset.h"
#include "Particles/ParticleSystemInstance.h"

void ParticleSystemEmitterNode::BeginPlay() {
    Super::BeginPlay();
    SetUpdateFlag(UpdateFlag::WorldUpdate);
    if (m_AutoPlay) {
        Play();
    }
}

void ParticleSystemEmitterNode::EndPlay() {
    if (m_DetachOnDestroy) {
        DetachInstance();
    } else if (World* world = GetWorld()) {
        DestroyInstance(*world);
    }
    Super::EndPlay();
}

void ParticleSystemEmitterNode::UnInitializeNode(World& OutWorld) {
    DestroyInstance(OutWorld);
    Super::UnInitializeNode(OutWorld);
}

void ParticleSystemEmitterNode::DestroyInstance(World& InWorld) {
    if (ParticleWorld* particles = InWorld.GetSubsystem<ParticleWorld>()) {
        particles->DestroyInstance(m_Instance);
    }
    m_Instance = ParticleHandle();
    m_WasAlive = false;
}

void ParticleSystemEmitterNode::DetachInstance() {
    if (ParticleSystemInstance* instance = m_Instance.Get()) {
        instance->Stop();
        instance->SetReleaseWhenFinished(true);
    }
    m_Instance = ParticleHandle();
    m_WasAlive = false;
}

void ParticleSystemEmitterNode::WorldUpdate(float InDeltaTime) {
    Super::WorldUpdate(InDeltaTime);

    ParticleSystemInstance* instance = m_Instance.Get();
    if (!instance) {
        return;
    }
    instance->SetTransform(GetTransformMatrix());

    const bool alive = instance->IsAlive();
    if (m_WasAlive && !alive) {
        if (OnFinished) {
            OnFinished();
        }
        if (m_StopAction == ParticleStopAction::Destroy) {
            Destroy();
            return;
        }
    }
    m_WasAlive = alive;
}

void ParticleSystemEmitterNode::Play() {
    if (ParticleSystemInstance* instance = EnsureInstance()) {
        instance->SetTransform(GetTransformMatrix());
        instance->Play();
        m_WasAlive = instance->IsAlive();
    }
}

void ParticleSystemEmitterNode::Stop(ParticleStopBehavior InBehavior) {
    m_Instance.Stop(InBehavior);
}

void ParticleSystemEmitterNode::SetPaused(bool InPaused) {
    m_Instance.SetPaused(InPaused);
}

void ParticleSystemEmitterNode::Clear() {
    m_Instance.Clear();
}

void ParticleSystemEmitterNode::Restart() {
    if (ParticleSystemInstance* instance = EnsureInstance()) {
        instance->SetTransform(GetTransformMatrix());
        instance->Restart();
        m_WasAlive = instance->IsAlive();
    }
}

void ParticleSystemEmitterNode::Emit(int32_t InCount, const ParticleEmitParams& InParams) {
    if (ParticleSystemInstance* instance = EnsureInstance()) {
        instance->SetTransform(GetTransformMatrix());
        instance->Emit(InCount, InParams);
        m_WasAlive = m_WasAlive || instance->IsAlive();
    }
}

ParticleSystemAsset* ParticleSystemEmitterNode::GetSystem() const {
    return m_System.Get();
}

void ParticleSystemEmitterNode::SetSystem(ParticleSystemAsset* InSystem) {
    if (GetSystem() == InSystem) {
        return;
    }
    m_System = InSystem;

    ParticleSystemInstance* instance = m_Instance.Get();
    if (!instance) {
        return;
    }
    const bool wasEmitting = instance->IsEmitting();
    instance->SetSystem(InSystem);
    if (wasEmitting) {
        instance->Play();
    }
}

void ParticleSystemEmitterNode::SetSimulationSpeed(float InSpeed) {
    m_SimulationSpeed = InSpeed;
    m_Instance.SetSimulationSpeed(InSpeed);
}

ParticleWorld* ParticleSystemEmitterNode::GetParticleWorld() const {
    World* world = GetWorld();
    return world ? world->GetSubsystem<ParticleWorld>() : nullptr;
}

ParticleSystemInstance* ParticleSystemEmitterNode::EnsureInstance() {
    if (ParticleSystemInstance* instance = m_Instance.Get()) {
        return instance;
    }
    ParticleWorld* particles = GetParticleWorld();
    if (!particles || !GetSystem()) {
        return nullptr;
    }
    m_Instance = particles->CreateInstance(GetSystem(), GetTransformMatrix());
    m_Instance.SetSimulationSpeed(m_SimulationSpeed);
    return m_Instance.Get();
}

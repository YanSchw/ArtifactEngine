#include "ParticleSystemInstance.h"
#include "Assets/ParticleSystemAsset.h"

#include <atomic>
#include <cmath>

static constexpr float s_MaxStep = 1.0f / 20.0f;
static constexpr int32_t s_MaxStepsPerFrame = 20;

static uint32_t NextAutoSeed() {
    static std::atomic<uint32_t> s_Counter{ 0x9E3779B9u };
    return s_Counter.fetch_add(0x9E3779B9u);
}

ParticleSystemAsset* ParticleSystemInstance::GetSystem() const {
    return m_System.Get();
}

void ParticleSystemInstance::SetSystem(ParticleSystemAsset* InSystem) {
    if (GetSystem() == InSystem) {
        return;
    }
    m_Paused = false;
    m_Emitters.Clear();
    m_System = InSystem;
    m_SystemHandle = InSystem ? InSystem->GetStreamHandle() : AssetStreamHandle();
}

void ParticleSystemInstance::Play() {
    ParticleSystemAsset* system = GetSystem();
    if (!system) {
        return;
    }
    if (m_Paused) {
        m_Paused = false;
        if (IsEmitting()) {
            return;
        }
    }
    if (!IsEmitting()) {
        PlayEmitters(*system, NextAutoSeed());
    }
}

void ParticleSystemInstance::Stop(ParticleStopBehavior InBehavior) {
    m_Paused = false;
    for (ParticleEmitterInstance& emitter : m_Emitters) {
        emitter.Stop();
    }
    if (InBehavior == ParticleStopBehavior::StopEmittingAndClear) {
        Clear();
    }
}

void ParticleSystemInstance::SetPaused(bool InPaused) {
    m_Paused = InPaused;
}

void ParticleSystemInstance::Clear() {
    for (ParticleEmitterInstance& emitter : m_Emitters) {
        emitter.Clear();
    }
}

void ParticleSystemInstance::Restart() {
    Stop(ParticleStopBehavior::StopEmittingAndClear);
    Play();
}

void ParticleSystemInstance::Emit(int32_t InCount, const ParticleEmitParams& InParams) {
    ParticleSystemAsset* system = GetSystem();
    if (!system || InCount <= 0) {
        return;
    }
    SyncEmitters(*system);
    const Array<ParticleEmitter>& emitters = system->GetEmitters();
    for (int32_t i = 0; i < m_Emitters.Size(); i++) {
        const bool target = InParams.Emitter < 0 ? !system->IsSubEmitter(i) : InParams.Emitter == i;
        if (target && emitters[i].Enabled) {
            m_Emitters[i].Emit(emitters[i], m_Transform, InCount, InParams);
        }
    }
    m_WasAlive = m_WasAlive || IsAlive();
}

void ParticleSystemInstance::Seek(float InTime) {
    ParticleSystemAsset* system = GetSystem();
    if (!system) {
        return;
    }
    Stop(ParticleStopBehavior::StopEmittingAndClear);
    PlayEmitters(*system, m_Seed);

    const float time = glm::max(InTime, 0.0f);
    const int32_t steps = (int32_t)std::ceil(time / s_MaxStep);
    for (int32_t i = 0; i < steps; i++) {
        Step(*system, time / (float)steps, m_Transform, m_Transform, {});
    }
    m_PlaybackTime = time;
}

void ParticleSystemInstance::Simulate(float InDeltaTime, const ParticleRayCast& InRayCast) {
    ParticleSystemAsset* system = GetSystem();
    if (!m_HasTransformHistory) {
        m_PreviousTransform = m_Transform;
        m_HasTransformHistory = true;
    }

    const float total = glm::min(InDeltaTime * m_SimulationSpeed, s_MaxStep * (float)s_MaxStepsPerFrame);
    if (system && !m_Paused && total > 0.0f) {
        SyncEmitters(*system);
        const int32_t steps = glm::max((int32_t)std::ceil(total / s_MaxStep), 1);
        for (int32_t i = 0; i < steps; i++) {
            Step(*system, total / (float)steps,
                 ParticleEmitterInstance::BlendTranslation(m_PreviousTransform, m_Transform, (float)i / (float)steps),
                 ParticleEmitterInstance::BlendTranslation(m_PreviousTransform, m_Transform, (float)(i + 1) / (float)steps), InRayCast);
        }
        m_PlaybackTime += total;
    }
    m_PreviousTransform = m_Transform;

    const bool alive = IsAlive();
    if (m_WasAlive && !alive && OnFinished) {
        OnFinished();
    }
    m_WasAlive = alive;
}

bool ParticleSystemInstance::IsAlive() const {
    for (int32_t i = 0; i < m_Emitters.Size(); i++) {
        if (IsEmitterActive(i) && m_Emitters[i].IsAlive()) {
            return true;
        }
    }
    return false;
}

bool ParticleSystemInstance::IsEmitting() const {
    for (int32_t i = 0; i < m_Emitters.Size(); i++) {
        if (IsEmitterActive(i) && m_Emitters[i].IsEmitting()) {
            return true;
        }
    }
    return false;
}

int32_t ParticleSystemInstance::GetParticleCount() const {
    int32_t count = 0;
    for (const ParticleEmitterInstance& emitter : m_Emitters) {
        count += emitter.GetParticles().Size();
    }
    return count;
}

bool ParticleSystemInstance::IsEmitterActive(int32_t InIndex) const {
    const ParticleSystemAsset* system = GetSystem();
    return system && InIndex < system->GetEmitters().Size() && system->GetEmitters()[InIndex].Enabled;
}

void ParticleSystemInstance::PlayEmitters(const ParticleSystemAsset& InSystem, uint32_t InSeed) {
    const Array<ParticleEmitter>& emitters = InSystem.GetEmitters();
    m_Seed = InSeed;
    m_PlaybackTime = 0.0f;
    m_Emitters.Resize(emitters.Size());
    for (int32_t i = 0; i < emitters.Size(); i++) {
        m_Emitters[i].Play(emitters[i], InSeed + (uint32_t)i * 0x9E3779B9u, !InSystem.IsSubEmitter(i), m_Transform);
    }
    m_PreviousTransform = m_Transform;
    m_HasTransformHistory = true;
    m_WasAlive = true;
}

void ParticleSystemInstance::SyncEmitters(const ParticleSystemAsset& InSystem) {
    if (m_Emitters.Size() == InSystem.GetEmitters().Size()) {
        return;
    }
    const bool emitting = IsEmitting();
    m_Emitters.Clear();
    m_Emitters.Resize(InSystem.GetEmitters().Size());
    if (emitting) {
        PlayEmitters(InSystem, m_Seed);
    }
}

void ParticleSystemInstance::Step(const ParticleSystemAsset& InSystem, float InDeltaTime, const Mat4& InFrom, const Mat4& InTo,
                                  const ParticleRayCast& InRayCast) {
    const Array<ParticleEmitter>& emitters = InSystem.GetEmitters();
    for (int32_t i = 0; i < m_Emitters.Size(); i++) {
        if (!emitters[i].Enabled) {
            m_Emitters[i].Clear();
            continue;
        }
        m_Emitters[i].Step(emitters[i], InDeltaTime * glm::max(emitters[i].Main.SimulationSpeed, 0.0f), InFrom, InTo, InRayCast);
    }
    ProcessSubEmits(InSystem, InTo);
}

void ParticleSystemInstance::ProcessSubEmits(const ParticleSystemAsset& InSystem, const Mat4& InTransform) {
    const Array<ParticleEmitter>& emitters = InSystem.GetEmitters();
    for (ParticleEmitterInstance& source : m_Emitters) {
        Array<ParticleSubEmit> pending;
        std::swap(pending, source.GetSubEmits());
        for (const ParticleSubEmit& emit : pending) {
            const int32_t target = emit.Params.Emitter;
            if (target >= 0 && target < m_Emitters.Size() && emitters[target].Enabled) {
                m_Emitters[target].Emit(emitters[target], InTransform, emit.Count, emit.Params);
            }
        }
    }
}

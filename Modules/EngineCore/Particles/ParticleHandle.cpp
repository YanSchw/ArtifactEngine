#include "ParticleHandle.h"
#include "ParticleSystemInstance.h"

ParticleHandle::ParticleHandle(ParticleSystemInstance* InInstance) : m_Instance(InInstance) {
}

ParticleSystemInstance* ParticleHandle::Get() const {
    return m_Instance.Get();
}

bool ParticleHandle::IsAlive() const {
    ParticleSystemInstance* instance = Get();
    return instance && instance->IsAlive();
}

bool ParticleHandle::IsPlaying() const {
    ParticleSystemInstance* instance = Get();
    return instance && instance->IsPlaying();
}

bool ParticleHandle::IsEmitting() const {
    ParticleSystemInstance* instance = Get();
    return instance && instance->IsEmitting();
}

bool ParticleHandle::IsPaused() const {
    ParticleSystemInstance* instance = Get();
    return instance && instance->IsPaused();
}

int32_t ParticleHandle::GetParticleCount() const {
    ParticleSystemInstance* instance = Get();
    return instance ? instance->GetParticleCount() : 0;
}

void ParticleHandle::Play() const {
    if (ParticleSystemInstance* instance = Get()) {
        instance->Play();
    }
}

void ParticleHandle::Stop(ParticleStopBehavior InBehavior) const {
    if (ParticleSystemInstance* instance = Get()) {
        instance->Stop(InBehavior);
    }
}

void ParticleHandle::SetPaused(bool InPaused) const {
    if (ParticleSystemInstance* instance = Get()) {
        instance->SetPaused(InPaused);
    }
}

void ParticleHandle::Clear() const {
    if (ParticleSystemInstance* instance = Get()) {
        instance->Clear();
    }
}

void ParticleHandle::Restart() const {
    if (ParticleSystemInstance* instance = Get()) {
        instance->Restart();
    }
}

void ParticleHandle::Emit(int32_t InCount, const ParticleEmitParams& InParams) const {
    if (ParticleSystemInstance* instance = Get()) {
        instance->Emit(InCount, InParams);
    }
}

void ParticleHandle::Seek(float InTime) const {
    if (ParticleSystemInstance* instance = Get()) {
        instance->Seek(InTime);
    }
}

void ParticleHandle::SetTransform(const Mat4& InTransform) const {
    if (ParticleSystemInstance* instance = Get()) {
        instance->SetTransform(InTransform);
    }
}

void ParticleHandle::SetSimulationSpeed(float InSpeed) const {
    if (ParticleSystemInstance* instance = Get()) {
        instance->SetSimulationSpeed(InSpeed);
    }
}

void ParticleHandle::SetOnFinished(std::function<void()> InCallback) const {
    if (ParticleSystemInstance* instance = Get()) {
        instance->OnFinished = std::move(InCallback);
    }
}

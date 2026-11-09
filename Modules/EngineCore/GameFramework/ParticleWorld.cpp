#include "ParticleWorld.h"
#include "Assets/ParticleSystemAsset.h"
#include "Core/MainThread.h"
#include "Particles/ParticleSystemInstance.h"

static ParticleWorldRayCast s_RayCast = nullptr;

void ParticleWorld::SetRayCast(ParticleWorldRayCast InRayCast) {
    s_RayCast = InRayCast;
}

void ParticleWorld::LateWorldUpdate(float InDeltaTime) {
    Simulate(InDeltaTime);
}

void ParticleWorld::Shutdown() {
    m_Instances.Clear();
}

ParticleHandle ParticleWorld::Emit(ParticleSystemAsset* InSystem, const Mat4& InTransform) {
    const ParticleHandle handle = CreateInstance(InSystem, InTransform);
    if (ParticleSystemInstance* instance = handle.Get()) {
        instance->SetReleaseWhenFinished(true);
        instance->Play();
    }
    return handle;
}

ParticleHandle ParticleWorld::CreateInstance(ParticleSystemAsset* InSystem, const Mat4& InTransform) {
    AE_ASSERT_MAIN_THREAD("ParticleWorld::CreateInstance");
    if (!InSystem) {
        return {};
    }

    ParticleSystemInstance* instance = Object::Create<ParticleSystemInstance>();
    instance->SetTransform(InTransform);
    instance->SetSystem(InSystem);
    m_Instances.Add(SharedObjectPtr<ParticleSystemInstance>(instance));
    return ParticleHandle(instance);
}

void ParticleWorld::DestroyInstance(const ParticleHandle& InHandle) {
    AE_ASSERT_MAIN_THREAD("ParticleWorld::DestroyInstance");
    ParticleSystemInstance* instance = InHandle.Get();
    if (!instance) {
        return;
    }
    for (int32_t i = 0; i < m_Instances.Size(); i++) {
        if (m_Instances[i].Get() == instance) {
            m_Instances.RemoveAt(i);
            return;
        }
    }
}

void ParticleWorld::Simulate(float InDeltaTime) {
    ParticleRayCast rayCast;
    if (s_RayCast) {
        rayCast = [this](const Vec3& InFrom, const Vec3& InTo, Vec3& OutPosition, Vec3& OutNormal) {
            return s_RayCast(GetWorld(), InFrom, InTo, OutPosition, OutNormal);
        };
    }
    for (int32_t i = 0; i < m_Instances.Size(); i++) {
        m_Instances[i]->Simulate(InDeltaTime, rayCast);
    }
    m_Instances.Filter([](const SharedObjectPtr<ParticleSystemInstance>& InInstance) {
        return InInstance.Get() && (!InInstance->GetReleaseWhenFinished() || InInstance->IsAlive());
    });
}

void ParticleWorld::StopAll(ParticleStopBehavior InBehavior) {
    for (const SharedObjectPtr<ParticleSystemInstance>& instance : m_Instances) {
        instance->Stop(InBehavior);
    }
}

int32_t ParticleWorld::GetParticleCount() const {
    int32_t count = 0;
    for (const SharedObjectPtr<ParticleSystemInstance>& instance : m_Instances) {
        count += instance->GetParticleCount();
    }
    return count;
}

#pragma once
#include "WorldSubsystem.h"
#include "Common/Array.h"
#include "Common/Types.h"
#include "Object/Pointer.h"
#include "Particles/ParticleHandle.h"
#include "ParticleWorld.gen.h"

class ParticleSystemAsset;
class ParticleSystemInstance;

using ParticleWorldRayCast = bool (*)(World& InWorld, const Vec3& InFrom, const Vec3& InTo, Vec3& OutPosition, Vec3& OutNormal);

class ParticleWorld : public WorldSubsystem {
public:
    ARTIFACT_CLASS();

    /** Lets particle collision query the physics module without depending on it. */
    static void SetRayCast(ParticleWorldRayCast InRayCast);

    virtual void LateWorldUpdate(float InDeltaTime) override;
    virtual void Shutdown() override;

    ParticleHandle Emit(ParticleSystemAsset* InSystem, const Mat4& InTransform);
    ParticleHandle CreateInstance(ParticleSystemAsset* InSystem, const Mat4& InTransform);
    void DestroyInstance(const ParticleHandle& InHandle);

    void Simulate(float InDeltaTime);
    void StopAll(ParticleStopBehavior InBehavior = ParticleStopBehavior::StopEmitting);

    const Array<SharedObjectPtr<ParticleSystemInstance>>& GetInstances() const { return m_Instances; }
    int32_t GetParticleCount() const;

private:
    Array<SharedObjectPtr<ParticleSystemInstance>> m_Instances;
};

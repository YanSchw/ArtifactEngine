#pragma once
#include "Common/Types.h"
#include "Object/Pointer.h"
#include <functional>
#include <optional>

class ParticleSystemInstance;

enum class ParticleStopBehavior : uint8_t {
    StopEmitting = 0,
    StopEmittingAndClear = 1
};

/** Positions and velocities are in world space. */
struct ParticleEmitParams {
    /** -1 emits from every emitter that is not a sub emitter. */
    int32_t Emitter = -1;
    std::optional<Vec3> Position;
    std::optional<Vec3> Velocity;
    std::optional<Color> StartColor;
    std::optional<float> StartSize;
    std::optional<float> StartLifetime;
    Vec3 InheritedVelocity = Vec3(0.0f);
};

struct ParticleHandle {
    ParticleHandle() = default;
    ParticleHandle(ParticleSystemInstance* InInstance);

    ParticleSystemInstance* Get() const;
    bool IsValid() const { return Get() != nullptr; }

    bool IsAlive() const;
    bool IsPlaying() const;
    bool IsEmitting() const;
    bool IsPaused() const;
    int32_t GetParticleCount() const;

    void Play() const;
    void Stop(ParticleStopBehavior InBehavior = ParticleStopBehavior::StopEmitting) const;
    void SetPaused(bool InPaused) const;
    void Clear() const;
    void Restart() const;
    void Emit(int32_t InCount, const ParticleEmitParams& InParams = {}) const;
    void Seek(float InTime) const;

    void SetTransform(const Mat4& InTransform) const;
    void SetSimulationSpeed(float InSpeed) const;
    void SetOnFinished(std::function<void()> InCallback) const;

private:
    WeakObjectPtr<ParticleSystemInstance> m_Instance;
};

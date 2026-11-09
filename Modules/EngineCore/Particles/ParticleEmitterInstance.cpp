#include "ParticleEmitterInstance.h"

#include <glm/gtc/constants.hpp>
#include <cmath>

static constexpr float s_MaxStep = 1.0f / 20.0f;
static constexpr float s_MinDuration = 0.05f;
static constexpr float s_Gravity = 9.81f;

struct ParticleEmitterInstance::EmitStep {
    float Start = 0.0f;
    float DeltaTime = 0.0f;
    Mat4 From = Mat4(1.0f);
    Mat4 To = Mat4(1.0f);

    float AlphaAt(float InStepTime) const {
        return DeltaTime > 0.0f ? glm::clamp((InStepTime - Start) / DeltaTime, 0.0f, 1.0f) : 1.0f;
    }
};

static uint32_t HashSeed(uint32_t InValue) {
    const uint32_t state = InValue * 747796405u + 2891336453u;
    const uint32_t word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

static float LatticeValue(int32_t InX, int32_t InY, int32_t InZ) {
    const uint32_t hash = HashSeed((uint32_t)InX * 73856093u ^ (uint32_t)InY * 19349663u ^ (uint32_t)InZ * 83492791u);
    return (float)(hash & 0xFFFFFFu) / (float)0xFFFFFFu * 2.0f - 1.0f;
}

static float ValueNoise(const Vec3& InPoint) {
    const Vec3 cell = glm::floor(InPoint);
    const Vec3 fraction = InPoint - cell;
    const Vec3 blend = fraction * fraction * (3.0f - 2.0f * fraction);
    const int32_t x = (int32_t)cell.x, y = (int32_t)cell.y, z = (int32_t)cell.z;

    const float x00 = glm::mix(LatticeValue(x, y, z), LatticeValue(x + 1, y, z), blend.x);
    const float x10 = glm::mix(LatticeValue(x, y + 1, z), LatticeValue(x + 1, y + 1, z), blend.x);
    const float x01 = glm::mix(LatticeValue(x, y, z + 1), LatticeValue(x + 1, y, z + 1), blend.x);
    const float x11 = glm::mix(LatticeValue(x, y + 1, z + 1), LatticeValue(x + 1, y + 1, z + 1), blend.x);
    return glm::mix(glm::mix(x00, x10, blend.y), glm::mix(x01, x11, blend.y), blend.z);
}

static Vec3 NoiseVector(const Vec3& InPoint) {
    return Vec3(ValueNoise(InPoint),
                ValueNoise(InPoint + Vec3(31.416f, -47.853f, 12.679f)),
                ValueNoise(InPoint + Vec3(-19.123f, 7.771f, 61.337f)));
}

/** Divergence free, so particles swirl instead of bunching up. */
static Vec3 CurlNoise(const Vec3& InPoint, int32_t InOctaves) {
    static constexpr float s_Epsilon = 0.05f;
    Vec3 curl(0.0f);
    float frequency = 1.0f;
    float amplitude = 1.0f;
    for (int32_t octave = 0; octave < glm::max(InOctaves, 1); octave++) {
        const Vec3 point = InPoint * frequency;
        const Vec3 dx = NoiseVector(point + Vec3(s_Epsilon, 0.0f, 0.0f)) - NoiseVector(point - Vec3(s_Epsilon, 0.0f, 0.0f));
        const Vec3 dy = NoiseVector(point + Vec3(0.0f, s_Epsilon, 0.0f)) - NoiseVector(point - Vec3(0.0f, s_Epsilon, 0.0f));
        const Vec3 dz = NoiseVector(point + Vec3(0.0f, 0.0f, s_Epsilon)) - NoiseVector(point - Vec3(0.0f, 0.0f, s_Epsilon));
        curl += Vec3(dy.z - dz.y, dz.x - dx.z, dx.y - dy.x) * (amplitude / (2.0f * s_Epsilon));
        frequency *= 2.0f;
        amplitude *= 0.5f;
    }
    return curl;
}

static Mat3 SafeInverse(const Mat3& InMatrix) {
    return std::abs(glm::determinant(InMatrix)) > 1e-12f ? glm::inverse(InMatrix) : Mat3(1.0f);
}

static Mat3 RotationOf(const Mat3& InBasis) {
    Mat3 rotation;
    for (int32_t axis = 0; axis < 3; axis++) {
        const float length = glm::length(InBasis[axis]);
        rotation[axis] = length > 1e-8f ? InBasis[axis] / length : Vec3(0.0f);
    }
    return rotation;
}

static Vec3 SafeNormalize(const Vec3& InVector, const Vec3& InFallback) {
    const float length = glm::length(InVector);
    return length > 1e-8f ? InVector / length : InFallback;
}

static float SpeedAlpha(const ParticleFloatRange& InRange, float InSpeed) {
    return glm::clamp((InSpeed - InRange.Min) / glm::max(InRange.Max - InRange.Min, 1e-4f), 0.0f, 1.0f);
}

static void RecordTrail(const ParticleTrailModule& InTrails, Particle& OutParticle) {
    while (!OutParticle.Trail.IsEmpty() && OutParticle.Age - OutParticle.Trail.FirstItem().w > InTrails.Lifetime) {
        OutParticle.Trail.RemoveFirstItem();
    }
    const float spacing = glm::max(InTrails.MinVertexDistance, 1e-3f);
    if (OutParticle.Trail.IsEmpty() || glm::distance(Vec3(OutParticle.Trail.LastItem()), OutParticle.Position) >= spacing) {
        OutParticle.Trail.Add(Vec4(OutParticle.Position, OutParticle.Age));
    }
}

Mat4 ParticleEmitterInstance::BlendTranslation(const Mat4& InFrom, const Mat4& InTo, float InAlpha) {
    Mat4 blended = InTo;
    blended[3] = glm::mix(InFrom[3], InTo[3], InAlpha);
    return blended;
}

float ParticleEmitterInstance::AverageScale(const Mat3& InBasis) {
    return (glm::length(InBasis[0]) + glm::length(InBasis[1]) + glm::length(InBasis[2])) / 3.0f;
}

void ParticleEmitterInstance::Play(const ParticleEmitter& InEmitter, uint32_t InSeed, bool InEmitting, const Mat4& InTransform) {
    const ParticleMainModule& main = InEmitter.Main;
    ApplySpace(InEmitter);
    m_RandomState = HashSeed(main.AutoRandomSeed ? InSeed : main.RandomSeed) | 1u;
    m_Time = -glm::max(main.StartDelay.Evaluate(Random01()), 0.0f);
    m_RateRemainder = 0.0f;
    m_DistanceRemainder = 0.0f;
    m_BurstCyclesFired.Clear();
    m_EmitterVelocity = Vec3(0.0f);
    m_Emitting = InEmitting;

    if (m_Emitting && main.Prewarm && main.Looping) {
        Prewarm(InEmitter, InTransform);
    }
}

void ParticleEmitterInstance::Emit(const ParticleEmitter& InEmitter, const Mat4& InTransform, int32_t InCount, const ParticleEmitParams& InParams) {
    ApplySpace(InEmitter);
    for (int32_t i = 0; i < InCount; i++) {
        if (!Spawn(InEmitter, InTransform, 0.0f, InParams)) {
            return;
        }
    }
}

void ParticleEmitterInstance::Step(const ParticleEmitter& InEmitter, float InDeltaTime, const Mat4& InFrom, const Mat4& InTo,
                                   const ParticleRayCast& InRayCast) {
    if (InDeltaTime <= 0.0f) {
        return;
    }
    ApplySpace(InEmitter);
    m_TotalTime += InDeltaTime;
    m_EmitterVelocity = (Vec3(InTo[3]) - Vec3(InFrom[3])) / InDeltaTime;
    UpdateParticles(InEmitter, InDeltaTime, InTo, InRayCast);
    if (m_Emitting) {
        EmitStep step;
        step.Start = m_Time;
        step.DeltaTime = InDeltaTime;
        step.From = InFrom;
        step.To = InTo;
        EmitOverStep(InEmitter, step);
    }
}

Vec3 ParticleEmitterInstance::ToWorldPosition(const Vec3& InPosition, const Mat4& InTransform) const {
    return m_Space == ParticleSimulationSpace::Local ? Vec3(InTransform * Vec4(InPosition, 1.0f)) : InPosition;
}

Vec3 ParticleEmitterInstance::ToWorldDirection(const Vec3& InDirection, const Mat4& InTransform) const {
    return m_Space == ParticleSimulationSpace::Local ? Mat3(InTransform) * InDirection : InDirection;
}

void ParticleEmitterInstance::UpdateParticles(const ParticleEmitter& InEmitter, float InDeltaTime, const Mat4& InTransform,
                                              const ParticleRayCast& InRayCast) {
    if (m_Particles.IsEmpty()) {
        return;
    }

    const bool local = m_Space == ParticleSimulationSpace::Local;
    const Mat3 basis = Mat3(InTransform);
    const Mat3 toLocal = SafeInverse(basis);
    const auto intoSimulation = [&](const Vec3& InVector, ParticleSimulationSpace InSpace) {
        if (InSpace == m_Space) {
            return InVector;
        }
        return InSpace == ParticleSimulationSpace::World ? toLocal * InVector : basis * InVector;
    };

    const ParticleMainModule& main = InEmitter.Main;
    const ParticleForceOverLifetimeModule& force = InEmitter.ForceOverLifetime;
    const ParticleVelocityOverLifetimeModule& velocity = InEmitter.VelocityOverLifetime;
    const ParticleLimitVelocityOverLifetimeModule& limit = InEmitter.LimitVelocityOverLifetime;
    const ParticleNoiseModule& noise = InEmitter.Noise;
    const ParticleCollisionModule& collision = InEmitter.Collision;
    const ParticleRotationBySpeedModule& rotationBySpeed = InEmitter.RotationBySpeed;
    const bool rotate = InEmitter.RotationOverLifetime.Enabled;
    const bool trails = InEmitter.Trails.Enabled && InEmitter.Trails.Mode == ParticleTrailMode::PerParticle;

    Vec3 acceleration = intoSimulation(Vec3(0.0f, -s_Gravity * main.GravityModifier, 0.0f), ParticleSimulationSpace::World);
    float dragFactor = 1.0f;
    if (force.Enabled) {
        acceleration += intoSimulation(force.Force, force.Space);
        dragFactor = 1.0f / (1.0f + glm::max(force.Drag, 0.0f) * InDeltaTime);
    }

    const Vec3 linear = velocity.Enabled ? intoSimulation(velocity.Linear, velocity.Space) : Vec3(0.0f);
    const Vec3 orbital = velocity.Enabled ? (local ? glm::radians(velocity.Orbital) : RotationOf(basis) * glm::radians(velocity.Orbital)) : Vec3(0.0f);
    const float orbitalSpeed = glm::length(orbital);
    const Vec3 orbitalAxis = SafeNormalize(orbital, VecUtils::Up);
    const Vec3 center = local ? Vec3(0.0f) : Vec3(InTransform[3]);
    const Vec3 scroll = Vec3(m_TotalTime * noise.ScrollSpeed);

    m_Planes.Clear();
    if (collision.Enabled && collision.Type == ParticleCollisionType::Planes) {
        for (const ParticleCollisionPlane& plane : collision.Planes) {
            const Vec3 normal = local ? plane.Normal : glm::transpose(toLocal) * plane.Normal;
            m_Planes.Add({ local ? plane.Position : Vec3(InTransform * Vec4(plane.Position, 1.0f)), SafeNormalize(normal, VecUtils::Up) });
        }
    }

    for (int32_t i = m_Particles.Size() - 1; i >= 0; i--) {
        Particle& particle = m_Particles[i];
        particle.Age += InDeltaTime;
        if (particle.Age >= particle.Lifetime) {
            Kill(InEmitter, i, InTransform);
            continue;
        }

        const float lifetime = particle.Age / particle.Lifetime;
        particle.Velocity += acceleration * InDeltaTime;
        particle.Velocity *= dragFactor;
        if (limit.Enabled) {
            const float maxSpeed = glm::max(limit.Speed * limit.SpeedOverLifetime.Evaluate(lifetime), 0.0f);
            const float speed = glm::length(particle.Velocity);
            if (speed > maxSpeed) {
                const float limited = maxSpeed + (speed - maxSpeed) * std::exp(-glm::max(limit.Dampen, 0.0f) * InDeltaTime);
                particle.Velocity *= limited / speed;
            }
        }

        Vec3 current = particle.Velocity;
        Vec3 orbit(0.0f);
        if (velocity.Enabled) {
            const Vec3 offset = particle.Position - center;
            current += linear;
            if (velocity.Radial != 0.0f && glm::dot(offset, offset) > 1e-8f) {
                current += glm::normalize(offset) * velocity.Radial;
            }
            const float modifier = velocity.SpeedModifier.Evaluate(lifetime);
            current *= modifier;
            if (orbitalSpeed > 0.0f) {
                orbit = glm::angleAxis(orbitalSpeed * modifier * InDeltaTime, orbitalAxis) * offset - offset;
            }
        }
        if (noise.Enabled) {
            current += CurlNoise(particle.Position * noise.Frequency + scroll, noise.Octaves) * noise.Strength;
        }

        const Vec3 previous = particle.Position;
        particle.Position += current * InDeltaTime + orbit;
        particle.CurrentVelocity = current + orbit / InDeltaTime;

        if (collision.Enabled && Collide(collision, particle, previous, InTransform, InRayCast)) {
            particle.Age += glm::max(collision.LifetimeLoss, 0.0f) * particle.Lifetime;
            TriggerSubEmitters(InEmitter, particle, ParticleSubEmitterTrigger::Collision, InTransform);
            if (particle.Age >= particle.Lifetime || glm::length(particle.Velocity) < collision.MinKillSpeed) {
                Kill(InEmitter, i, InTransform);
                continue;
            }
        }

        if (rotate) {
            particle.Rotation += particle.AngularVelocity * InDeltaTime;
        }
        if (rotationBySpeed.Enabled) {
            const float alpha = SpeedAlpha(rotationBySpeed.SpeedRange, glm::length(particle.CurrentVelocity));
            particle.Rotation += glm::radians(rotationBySpeed.AngularVelocity) * alpha * InDeltaTime;
        }
        if (trails) {
            RecordTrail(InEmitter.Trails, particle);
        }
        RefreshAppearance(InEmitter, particle);
        TriggerSubEmitters(InEmitter, particle, ParticleSubEmitterTrigger::Alive, InTransform, InDeltaTime);
    }
}

bool ParticleEmitterInstance::Collide(const ParticleCollisionModule& InCollision, Particle& OutParticle, const Vec3& InPrevious,
                                      const Mat4& InTransform, const ParticleRayCast& InRayCast) {
    const float radius = 0.5f * OutParticle.CurrentSize * glm::max(InCollision.RadiusScale, 0.0f);
    Vec3 normal(0.0f);
    bool hit = false;

    for (const ParticleCollisionPlane& plane : m_Planes) {
        const float distance = glm::dot(OutParticle.Position - plane.Position, plane.Normal);
        if (distance < radius && glm::dot(OutParticle.Velocity, plane.Normal) < 0.0f) {
            OutParticle.Position += plane.Normal * (radius - distance);
            normal = plane.Normal;
            hit = true;
            break;
        }
    }

    if (InCollision.Type == ParticleCollisionType::World && InRayCast) {
        const Vec3 from = ToWorldPosition(InPrevious, InTransform);
        const Vec3 to = ToWorldPosition(OutParticle.Position, InTransform);
        const Vec3 direction = to - from;
        const float length = glm::length(direction);
        Vec3 position;
        Vec3 worldNormal;
        if (length > 1e-6f && InRayCast(from, to + direction / length * radius, position, worldNormal)) {
            position += worldNormal * radius;
            const bool local = m_Space == ParticleSimulationSpace::Local;
            OutParticle.Position = local ? Vec3(glm::inverse(InTransform) * Vec4(position, 1.0f)) : position;
            normal = local ? SafeNormalize(glm::transpose(Mat3(InTransform)) * worldNormal, VecUtils::Up) : worldNormal;
            hit = true;
        }
    }

    if (!hit) {
        return false;
    }
    const float into = glm::dot(OutParticle.Velocity, normal);
    if (into < 0.0f) {
        OutParticle.Velocity -= (1.0f + glm::max(InCollision.Bounce, 0.0f)) * into * normal;
    }
    OutParticle.Velocity *= 1.0f - glm::clamp(InCollision.Dampen, 0.0f, 1.0f);
    return true;
}

void ParticleEmitterInstance::Kill(const ParticleEmitter& InEmitter, int32_t InIndex, const Mat4& InTransform) {
    TriggerSubEmitters(InEmitter, m_Particles[InIndex], ParticleSubEmitterTrigger::Death, InTransform);
    if (InIndex != m_Particles.Last()) {
        m_Particles[InIndex] = std::move(m_Particles.LastItem());
    }
    m_Particles.RemoveLastItem();
}

void ParticleEmitterInstance::TriggerSubEmitters(const ParticleEmitter& InEmitter, const Particle& InParticle, ParticleSubEmitterTrigger InTrigger,
                                                 const Mat4& InTransform, float InDeltaTime) {
    for (const ParticleSubEmitter& sub : InEmitter.SubEmitters) {
        if (!sub.Enabled || sub.Trigger != InTrigger) {
            continue;
        }
        float count = glm::max(sub.Count.Evaluate(Random01()), 0.0f);
        if (InTrigger == ParticleSubEmitterTrigger::Alive) {
            count *= InDeltaTime * sub.Probability;
        } else if (Random01() > sub.Probability) {
            continue;
        }
        const int32_t whole = (int32_t)count + (Random01() < count - std::floor(count) ? 1 : 0);
        if (whole <= 0) {
            continue;
        }

        ParticleSubEmit emit;
        emit.Count = whole;
        emit.Params.Emitter = sub.EmitterIndex;
        emit.Params.Position = ToWorldPosition(InParticle.Position, InTransform);
        emit.Params.InheritedVelocity = ToWorldDirection(InParticle.CurrentVelocity, InTransform) * sub.InheritVelocity;
        if (sub.InheritColor) {
            emit.Params.StartColor = InParticle.CurrentColor;
        }
        if (sub.InheritSize) {
            const bool world = m_Space == ParticleSimulationSpace::World;
            emit.Params.StartSize = InParticle.CurrentSize / (world ? glm::max(AverageScale(Mat3(InTransform)), 1e-6f) : 1.0f);
        }
        m_SubEmits.Add(emit);
    }
}

void ParticleEmitterInstance::EmitOverStep(const ParticleEmitter& InEmitter, const EmitStep& InStep) {
    const ParticleMainModule& main = InEmitter.Main;
    const ParticleEmissionModule& emission = InEmitter.Emission;
    const float duration = glm::max(main.Duration, s_MinDuration);
    const float end = InStep.Start + InStep.DeltaTime;
    m_Time = end;
    if (end <= 0.0f) {
        return;
    }

    const float activeFrom = glm::max(InStep.Start, 0.0f);
    const bool reachedEnd = end >= duration;

    EmitWindow(InEmitter, InStep, activeFrom, reachedEnd ? duration : end, 0.0f);
    if (reachedEnd) {
        if (main.Looping) {
            m_Time = end - duration;
            m_BurstCyclesFired.Clear();
            EmitWindow(InEmitter, InStep, 0.0f, m_Time, duration);
        } else {
            m_Time = duration;
            m_Emitting = false;
        }
    }

    if (emission.Enabled && emission.RateOverDistance > 0.0f) {
        const float distance = glm::length(Vec3(InStep.To[3]) - Vec3(InStep.From[3]));
        m_DistanceRemainder += distance * emission.RateOverDistance;
        const int32_t count = (int32_t)m_DistanceRemainder;
        m_DistanceRemainder -= (float)count;
        SpawnSpread(InEmitter, InStep, count, InStep.AlphaAt(activeFrom), 1.0f);
    }
}

void ParticleEmitterInstance::EmitWindow(const ParticleEmitter& InEmitter, const EmitStep& InStep, float InFrom, float InTo, float InCycleOffset) {
    const ParticleEmissionModule& emission = InEmitter.Emission;
    if (!emission.Enabled || InTo <= InFrom) {
        return;
    }

    const float rate = emission.RateOverTime * emission.RateOverTimeCurve.Evaluate(InFrom / glm::max(InEmitter.Main.Duration, s_MinDuration));
    if (rate > 0.0f) {
        m_RateRemainder += rate * (InTo - InFrom);
        const int32_t count = (int32_t)m_RateRemainder;
        m_RateRemainder -= (float)count;
        SpawnSpread(InEmitter, InStep, count, InStep.AlphaAt(InFrom + InCycleOffset), InStep.AlphaAt(InTo + InCycleOffset));
    }

    if (m_BurstCyclesFired.Size() != emission.Bursts.Size()) {
        m_BurstCyclesFired.Resize(emission.Bursts.Size());
    }
    for (int32_t index = 0; index < emission.Bursts.Size(); index++) {
        const ParticleBurst& burst = emission.Bursts[index];
        if (burst.Enabled) {
            FireBurst(InEmitter, InStep, burst, m_BurstCyclesFired[index], InFrom, InTo, InCycleOffset);
        }
    }
}

void ParticleEmitterInstance::FireBurst(const ParticleEmitter& InEmitter, const EmitStep& InStep, const ParticleBurst& InBurst,
                                        int32_t& InOutCyclesFired, float InFrom, float InTo, float InCycleOffset) {
    const float interval = glm::max(InBurst.Interval, 0.001f);
    while (InBurst.Cycles <= 0 || InOutCyclesFired < InBurst.Cycles) {
        const float fireTime = glm::max(InBurst.Time, 0.0f) + (float)InOutCyclesFired * interval;
        if (fireTime >= InTo) {
            return;
        }
        InOutCyclesFired++;
        if (Random01() > InBurst.Probability) {
            continue;
        }
        const int32_t count = (int32_t)std::round(glm::max(InBurst.Count.Evaluate(Random01()), 0.0f));
        const float alpha = InStep.AlphaAt(glm::max(fireTime, InFrom) + InCycleOffset);
        SpawnSpread(InEmitter, InStep, count, alpha, alpha);
    }
}

void ParticleEmitterInstance::SpawnSpread(const ParticleEmitter& InEmitter, const EmitStep& InStep, int32_t InCount, float InFromAlpha, float InToAlpha) {
    for (int32_t i = 0; i < InCount; i++) {
        const float alpha = glm::mix(InFromAlpha, InToAlpha, ((float)i + 0.5f) / (float)InCount);
        if (!Spawn(InEmitter, BlendTranslation(InStep.From, InStep.To, alpha), (1.0f - alpha) * InStep.DeltaTime, {})) {
            return;
        }
    }
}

bool ParticleEmitterInstance::Spawn(const ParticleEmitter& InEmitter, const Mat4& InTransform, float InPreAdvance, const ParticleEmitParams& InParams) {
    const ParticleMainModule& main = InEmitter.Main;
    if (m_Particles.Size() >= glm::max(main.MaxParticles, 0)) {
        return false;
    }

    Particle particle;
    particle.Lifetime = glm::max(InParams.StartLifetime.value_or(main.StartLifetime.Evaluate(Random01())), 0.01f);
    particle.StartSize = glm::max(InParams.StartSize.value_or(main.StartSize.Evaluate(Random01())), 0.0f);
    particle.Rotation = glm::radians(main.StartRotation.Evaluate(RandomVector()));
    particle.StartColor = InParams.StartColor.value_or(main.StartColor.Evaluate(Random01()));
    particle.AngularVelocity = glm::radians(InEmitter.RotationOverLifetime.AngularVelocity.Evaluate(RandomVector()));
    particle.Random = Random01();
    const float speed = main.StartSpeed.Evaluate(Random01());

    Vec3 position(0.0f);
    Vec3 direction = VecUtils::Up;
    SampleShape(InEmitter.Shape, position, direction);

    const Mat3 basis = Mat3(InTransform);
    const Vec3 origin = InParams.Position.value_or(Vec3(InTransform[3]));
    Vec3 inherited = InParams.InheritedVelocity;
    if (InEmitter.InheritVelocity.Enabled) {
        inherited += m_EmitterVelocity * InEmitter.InheritVelocity.Multiplier;
    }
    if (m_Space == ParticleSimulationSpace::World) {
        particle.Position = origin + basis * position;
        particle.Velocity = InParams.Velocity.value_or(SafeNormalize(basis * direction, VecUtils::Up) * speed) + inherited;
        particle.StartSize *= AverageScale(basis);
    } else {
        const Mat3 toLocal = SafeInverse(basis);
        particle.Position = position + toLocal * (origin - Vec3(InTransform[3]));
        particle.Velocity = (InParams.Velocity ? toLocal * *InParams.Velocity : direction * speed) + toLocal * inherited;
    }

    particle.CurrentVelocity = particle.Velocity;
    if (InPreAdvance > 0.0f) {
        if (InPreAdvance >= particle.Lifetime) {
            return true;
        }
        particle.Age = InPreAdvance;
        particle.Position += particle.Velocity * InPreAdvance;
    }
    RefreshAppearance(InEmitter, particle);
    if (InEmitter.Trails.Enabled && InEmitter.Trails.Mode == ParticleTrailMode::PerParticle) {
        RecordTrail(InEmitter.Trails, particle);
    }
    TriggerSubEmitters(InEmitter, particle, ParticleSubEmitterTrigger::Birth, InTransform);
    m_Particles.Add(std::move(particle));
    return true;
}

void ParticleEmitterInstance::SampleShape(const ParticleShapeModule& InShape, Vec3& OutPosition, Vec3& OutDirection) {
    OutPosition = Vec3(0.0f);
    OutDirection = VecUtils::Up;
    if (!InShape.Enabled) {
        return;
    }

    const float radius = glm::max(InShape.Radius, 0.0f);
    const float thickness = glm::clamp(InShape.RadiusThickness, 0.0f, 1.0f);
    const float arc = glm::radians(glm::clamp(InShape.Arc, 0.0f, 360.0f));

    switch (InShape.Shape) {
        case ParticleShapeType::Point:
            OutDirection = RandomUnitVector();
            break;
        case ParticleShapeType::Sphere:
        case ParticleShapeType::Hemisphere: {
            OutDirection = RandomUnitVector();
            if (InShape.Shape == ParticleShapeType::Hemisphere) {
                OutDirection.y = std::abs(OutDirection.y);
            }
            OutPosition = OutDirection * radius * glm::mix(1.0f, std::cbrt(Random01()), thickness);
            break;
        }
        case ParticleShapeType::Cone: {
            const float theta = Random01() * arc;
            const float distance = glm::mix(1.0f, std::sqrt(Random01()), thickness);
            const Vec3 radial(std::cos(theta), 0.0f, std::sin(theta));
            const float angle = glm::radians(glm::clamp(InShape.Angle, 0.0f, 90.0f));
            const float tilt = radius > 1e-4f ? angle * distance : angle * std::sqrt(Random01());
            OutPosition = radial * radius * distance;
            OutDirection = radial * std::sin(tilt) + VecUtils::Up * std::cos(tilt);
            break;
        }
        case ParticleShapeType::Box:
            OutPosition = (Vec3(Random01(), Random01(), Random01()) - 0.5f) * InShape.BoxSize;
            break;
        case ParticleShapeType::Circle: {
            const float theta = Random01() * arc;
            const Vec3 radial(std::cos(theta), 0.0f, std::sin(theta));
            OutPosition = radial * radius * glm::mix(1.0f, std::sqrt(Random01()), thickness);
            OutDirection = radial;
            break;
        }
        case ParticleShapeType::Edge:
            OutPosition = Vec3((Random01() * 2.0f - 1.0f) * radius, 0.0f, 0.0f);
            break;
    }

    const float randomize = glm::clamp(InShape.RandomizeDirection, 0.0f, 1.0f);
    if (randomize > 0.0f) {
        OutDirection = SafeNormalize(glm::mix(OutDirection, RandomUnitVector(), randomize), OutDirection);
    }

    const Quat rotation = Quat(glm::radians(InShape.Rotation));
    OutPosition = rotation * OutPosition + InShape.Position;
    OutDirection = rotation * OutDirection;
}

void ParticleEmitterInstance::RefreshAppearance(const ParticleEmitter& InEmitter, Particle& OutParticle) const {
    const float lifetime = glm::clamp(OutParticle.Age / OutParticle.Lifetime, 0.0f, 1.0f);
    const float speed = glm::length(OutParticle.CurrentVelocity);

    OutParticle.CurrentColor = OutParticle.StartColor;
    if (InEmitter.ColorOverLifetime.Enabled) {
        OutParticle.CurrentColor *= InEmitter.ColorOverLifetime.Gradient.Evaluate(lifetime);
    }
    if (InEmitter.ColorBySpeed.Enabled) {
        OutParticle.CurrentColor *= InEmitter.ColorBySpeed.Gradient.Evaluate(SpeedAlpha(InEmitter.ColorBySpeed.SpeedRange, speed));
    }

    OutParticle.CurrentSize = OutParticle.StartSize;
    if (InEmitter.SizeOverLifetime.Enabled) {
        OutParticle.CurrentSize *= glm::max(InEmitter.SizeOverLifetime.Size.Evaluate(lifetime), 0.0f);
    }
    if (InEmitter.SizeBySpeed.Enabled) {
        OutParticle.CurrentSize *= glm::max(InEmitter.SizeBySpeed.Size.Evaluate(SpeedAlpha(InEmitter.SizeBySpeed.SpeedRange, speed)), 0.0f);
    }
}

void ParticleEmitterInstance::Prewarm(const ParticleEmitter& InEmitter, const Mat4& InTransform) {
    const float duration = glm::max(InEmitter.Main.Duration, s_MinDuration);
    const int32_t steps = (int32_t)std::ceil(duration / s_MaxStep);
    const float step = duration / (float)steps;

    m_Time = 0.0f;
    for (int32_t i = 0; i < steps && m_Emitting; i++) {
        Step(InEmitter, step, InTransform, InTransform, {});
    }
    m_SubEmits.Clear();
}

void ParticleEmitterInstance::ApplySpace(const ParticleEmitter& InEmitter) {
    if (InEmitter.Main.SimulationSpace != m_Space) {
        m_Particles.Clear();
        m_Space = InEmitter.Main.SimulationSpace;
    }
}

uint32_t ParticleEmitterInstance::NextRandom() {
    uint32_t state = m_RandomState;
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    m_RandomState = state;
    return state;
}

float ParticleEmitterInstance::Random01() {
    return (float)(NextRandom() >> 8) * (1.0f / 16777216.0f);
}

Vec3 ParticleEmitterInstance::RandomVector() {
    const float x = Random01();
    const float y = Random01();
    return Vec3(x, y, Random01());
}

Vec3 ParticleEmitterInstance::RandomUnitVector() {
    const float z = Random01() * 2.0f - 1.0f;
    const float phi = Random01() * glm::two_pi<float>();
    const float ring = std::sqrt(glm::max(1.0f - z * z, 0.0f));
    return Vec3(ring * std::cos(phi), z, ring * std::sin(phi));
}

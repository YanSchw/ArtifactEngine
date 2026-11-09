#include "ParticlePreview.h"
#include "Assets/ParticleSystemAsset.h"
#include "GameFramework/DebugDraw.h"
#include "GameFramework/ParticleSystemEmitterNode.h"
#include "GameFramework/ParticleWorld.h"
#include "GameFramework/World.h"
#include "Particles/ParticleSystemInstance.h"

void ParticlePreview::Tick(World& InWorld, float InDeltaTime) {
    ParticleWorld* particles = InWorld.GetSubsystem<ParticleWorld>();
    if (!particles) {
        return;
    }
    for (Node* node : InWorld.GetAllNodes()) {
        if (ParticleSystemEmitterNode* emitter = Cast<ParticleSystemEmitterNode>(node)) {
            Sync(*emitter);
        }
    }
    particles->Simulate(InDeltaTime);
}

void ParticlePreview::Sync(ParticleSystemEmitterNode& InEmitter) {
    ParticleSystemInstance* instance = InEmitter.GetInstance().Get();
    if (!InEmitter.GetSystem() || !InEmitter.IsEnabled() || InEmitter.IsPendingKill()) {
        if (instance && instance->IsAlive()) {
            instance->Stop(ParticleStopBehavior::StopEmittingAndClear);
        }
        return;
    }
    if (!instance) {
        InEmitter.Play();
        return;
    }

    instance->SetTransform(InEmitter.GetTransformMatrix());
    instance->SetSimulationSpeed(InEmitter.GetSimulationSpeed());
    if (instance->GetSystem() != InEmitter.GetSystem()) {
        instance->SetSystem(InEmitter.GetSystem());
    }
    if (!instance->IsAlive()) {
        instance->Play();
    }
}

static const Color s_ShapeColor = Color(0.45f, 0.8f, 1.0f, 0.8f);

static void DrawCircle(World& InWorld, const Mat4& InTransform, float InRadius, float InHeight, float InArc) {
    static constexpr int32_t s_Segments = 32;
    Vec3 previous(0.0f);
    for (int32_t i = 0; i <= s_Segments; i++) {
        const float angle = InArc * (float)i / (float)s_Segments;
        const Vec3 point = Vec3(InTransform * Vec4(std::cos(angle) * InRadius, InHeight, std::sin(angle) * InRadius, 1.0f));
        if (i > 0) {
            DebugDraw::Line(&InWorld, previous, point, s_ShapeColor);
        }
        previous = point;
    }
}

void ParticlePreview::DrawShapes(World& InWorld, const ParticleSystemAsset& InSystem, const Mat4& InTransform) {
    for (const ParticleEmitter& emitter : InSystem.GetEmitters()) {
        const ParticleShapeModule& shape = emitter.Shape;
        if (!emitter.Enabled || !shape.Enabled) {
            continue;
        }
        const Mat4 transform = InTransform * glm::translate(Mat4(1.0f), shape.Position) * glm::mat4_cast(Quat(glm::radians(shape.Rotation)));
        const Vec3 center = Vec3(transform[3]);
        const float arc = glm::radians(glm::clamp(shape.Arc, 0.0f, 360.0f));
        const float scale = glm::length(Vec3(transform[0]));

        switch (shape.Shape) {
            case ParticleShapeType::Sphere:
            case ParticleShapeType::Hemisphere:
                DebugDraw::Sphere(&InWorld, center, shape.Radius * scale, s_ShapeColor);
                break;
            case ParticleShapeType::Cone: {
                const float top = shape.Radius + std::tan(glm::radians(glm::clamp(shape.Angle, 0.0f, 89.0f)));
                DrawCircle(InWorld, transform, shape.Radius, 0.0f, arc);
                DrawCircle(InWorld, transform, top, 1.0f, arc);
                for (int32_t side = 0; side < 4; side++) {
                    const float angle = arc * (float)side / 4.0f;
                    const Vec3 direction(std::cos(angle), 0.0f, std::sin(angle));
                    DebugDraw::Line(&InWorld, Vec3(transform * Vec4(direction * shape.Radius, 1.0f)),
                                    Vec3(transform * Vec4(direction * top + VecUtils::Up, 1.0f)), s_ShapeColor);
                }
                break;
            }
            case ParticleShapeType::Box:
                DebugDraw::Box(&InWorld, center, shape.BoxSize * 0.5f * scale, glm::quat_cast(Mat3(transform) / scale), s_ShapeColor);
                break;
            case ParticleShapeType::Circle:
                DrawCircle(InWorld, transform, shape.Radius, 0.0f, arc);
                break;
            case ParticleShapeType::Edge:
                DebugDraw::Line(&InWorld, Vec3(transform * Vec4(-shape.Radius, 0.0f, 0.0f, 1.0f)),
                                Vec3(transform * Vec4(shape.Radius, 0.0f, 0.0f, 1.0f)), s_ShapeColor);
                break;
            case ParticleShapeType::Point:
                break;
        }
    }
}

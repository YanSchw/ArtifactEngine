#include "DebugDraw.h"
#include "CameraNode.h"
#include "World.h"
#include "Rendering/Buffer.h"
#include "Rendering/FrameBuffer.h"
#include "Rendering/Pipeline.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/Shader.h"
#include "Rendering/ShaderLibrary.h"
#include "Rendering/VertexBuffer.h"

#include <glm/gtc/constants.hpp>
#include <cstring>
#include <cmath>

static SharedObjectPtr<Shader> s_DebugShader;
static Array<DebugDraw*> s_Instances;
static constexpr int32_t s_CircleSegments = 32;
static constexpr int32_t s_ArcSegments = 16;

struct DebugUniformData {
    Mat4 ViewProjection;
};

void DebugDraw::Initialize() {
    s_Instances.Add(this);
}

void DebugDraw::Shutdown() {
    s_Instances.Remove(this);
    m_Pipeline = nullptr;
    m_UniformBuffer = nullptr;
    m_VertexBuffer = nullptr;
}

DebugDraw* DebugDraw::Resolve(World* InWorld) {
#if defined(AE_TARGET_DIST)
    (void)InWorld;
    return nullptr;
#else
    return InWorld ? InWorld->GetSubsystem<DebugDraw>() : nullptr;
#endif
}

void DebugDraw::AdvanceFrame(float InDeltaTime) {
    for (DebugDraw* instance : s_Instances) {
        instance->Expire(InDeltaTime);
    }
}

void DebugDraw::Expire(float InDeltaTime) {
    for (int32_t i = m_Lines.Size() - 1; i >= 0; i--) {
        if (m_Lines[i].Remaining <= 0.0f) {
            m_Lines.RemoveAt(i);
        } else {
            m_Lines[i].Remaining -= InDeltaTime;
        }
    }
}

void DebugDraw::Push(const Vec3& InFrom, const Vec3& InTo, const Color& InColor, float InThickness, float InDuration) {
    m_Lines.Add(DebugLine{ InFrom, InTo, InColor, InThickness, glm::max(InDuration, 0.0f) });
}

void DebugDraw::PushArc(const Vec3& InCenter, const Vec3& InAxisU, const Vec3& InAxisV, float InRadius, float InFromAngle, float InToAngle, int32_t InSegments, const Color& InColor, float InThickness, float InDuration) {
    Vec3 previous = InCenter + (InAxisU * std::cos(InFromAngle) + InAxisV * std::sin(InFromAngle)) * InRadius;
    for (int32_t i = 1; i <= InSegments; i++) {
        const float angle = InFromAngle + (InToAngle - InFromAngle) * (float)i / (float)InSegments;
        const Vec3 current = InCenter + (InAxisU * std::cos(angle) + InAxisV * std::sin(angle)) * InRadius;
        Push(previous, current, InColor, InThickness, InDuration);
        previous = current;
    }
}

void DebugDraw::PushTube(const Vec3& InTop, const Vec3& InBottom, const Vec3& InAxisU, const Vec3& InAxisV, float InRadius, const Color& InColor, float InThickness, float InDuration) {
    PushArc(InTop, InAxisU, InAxisV, InRadius, 0.0f, glm::two_pi<float>(), s_CircleSegments, InColor, InThickness, InDuration);
    PushArc(InBottom, InAxisU, InAxisV, InRadius, 0.0f, glm::two_pi<float>(), s_CircleSegments, InColor, InThickness, InDuration);

    for (int32_t i = 0; i < 4; i++) {
        const float angle = glm::half_pi<float>() * (float)i;
        const Vec3 offset = (InAxisU * std::cos(angle) + InAxisV * std::sin(angle)) * InRadius;
        Push(InTop + offset, InBottom + offset, InColor, InThickness, InDuration);
    }
}

void DebugDraw::Line(World* InWorld, const Vec3& InFrom, const Vec3& InTo, const Color& InColor, float InThickness, float InDuration) {
    if (DebugDraw* debug = Resolve(InWorld)) {
        debug->Push(InFrom, InTo, InColor, InThickness, InDuration);
    }
}

void DebugDraw::Box(World* InWorld, const Vec3& InCenter, const Vec3& InHalfExtents, const Quat& InRotation, const Color& InColor, float InThickness, float InDuration) {
    DebugDraw* debug = Resolve(InWorld);
    if (!debug) {
        return;
    }

    const Vec3 axes[3] = { InRotation * Vec3(InHalfExtents.x, 0.0f, 0.0f),
                           InRotation * Vec3(0.0f, InHalfExtents.y, 0.0f),
                           InRotation * Vec3(0.0f, 0.0f, InHalfExtents.z) };

    Vec3 corners[8];
    for (int32_t corner = 0; corner < 8; corner++) {
        corners[corner] = InCenter;
        for (int32_t axis = 0; axis < 3; axis++) {
            corners[corner] += (corner & (1 << axis)) ? axes[axis] : -axes[axis];
        }
    }

    for (int32_t corner = 0; corner < 8; corner++) {
        for (int32_t axis = 0; axis < 3; axis++) {
            const int32_t neighbour = corner | (1 << axis);
            if (neighbour != corner) {
                debug->Push(corners[corner], corners[neighbour], InColor, InThickness, InDuration);
            }
        }
    }
}

void DebugDraw::Sphere(World* InWorld, const Vec3& InCenter, float InRadius, const Color& InColor, float InThickness, float InDuration) {
    DebugDraw* debug = Resolve(InWorld);
    if (!debug) {
        return;
    }

    const float full = glm::two_pi<float>();
    debug->PushArc(InCenter, VecUtils::Right, VecUtils::Up, InRadius, 0.0f, full, s_CircleSegments, InColor, InThickness, InDuration);
    debug->PushArc(InCenter, VecUtils::Up, VecUtils::Forward, InRadius, 0.0f, full, s_CircleSegments, InColor, InThickness, InDuration);
    debug->PushArc(InCenter, VecUtils::Forward, VecUtils::Right, InRadius, 0.0f, full, s_CircleSegments, InColor, InThickness, InDuration);
}

void DebugDraw::Capsule(World* InWorld, const Vec3& InCenter, float InHalfHeight, float InRadius, const Quat& InRotation, const Color& InColor, float InThickness, float InDuration) {
    DebugDraw* debug = Resolve(InWorld);
    if (!debug) {
        return;
    }

    const Vec3 up = InRotation * VecUtils::Up;
    const Vec3 right = InRotation * VecUtils::Right;
    const Vec3 forward = InRotation * VecUtils::Forward;
    const Vec3 top = InCenter + up * InHalfHeight;
    const Vec3 bottom = InCenter - up * InHalfHeight;

    debug->PushTube(top, bottom, right, forward, InRadius, InColor, InThickness, InDuration);

    const float half = glm::pi<float>();
    debug->PushArc(top, right, up, InRadius, 0.0f, half, s_ArcSegments, InColor, InThickness, InDuration);
    debug->PushArc(top, forward, up, InRadius, 0.0f, half, s_ArcSegments, InColor, InThickness, InDuration);
    debug->PushArc(bottom, right, up, InRadius, half, half * 2.0f, s_ArcSegments, InColor, InThickness, InDuration);
    debug->PushArc(bottom, forward, up, InRadius, half, half * 2.0f, s_ArcSegments, InColor, InThickness, InDuration);
}

void DebugDraw::Cylinder(World* InWorld, const Vec3& InCenter, float InHalfHeight, float InRadius,
                         const Quat& InRotation, const Color& InColor, float InThickness, float InDuration) {
    DebugDraw* debug = Resolve(InWorld);
    if (!debug) {
        return;
    }

    const Vec3 up = InRotation * VecUtils::Up;
    debug->PushTube(InCenter + up * InHalfHeight, InCenter - up * InHalfHeight,
                    InRotation * VecUtils::Right, InRotation * VecUtils::Forward,
                    InRadius, InColor, InThickness, InDuration);
}

#if !defined(AE_TARGET_DIST)

void DebugDraw::EnsurePipeline(FrameBuffer* InTarget) {
    if (!s_DebugShader.Get()) {
        s_DebugShader = ShaderLibrary::CreateShader("/Shaders/DebugLines.glsl");
        if (!s_DebugShader.Get()) {
            return;
        }
    }
    if (!m_UniformBuffer.Get()) {
        m_UniformBuffer = UniformBuffer::Create(0, sizeof(DebugUniformData));
    }
    if (!m_VertexBuffer.Get()) {
        m_VertexBuffer = VertexBuffer::CreateDynamic();
    }
    if (m_Pipeline.Get() && m_PipelineTarget.Get() == InTarget) {
        return;
    }
    if (m_Pipeline.Get()) {
        RenderingAPI::GetInstance()->WaitIdle();
    }

    PipelineDesc desc;
    desc.Target = InTarget;
    desc.Shader = s_DebugShader;
    desc.VertexLayout = { ShaderDataType::Float3, ShaderDataType::Float4 };
    desc.Buffers.Add(m_UniformBuffer);
    m_Pipeline = Pipeline::Create(desc);
    m_PipelineTarget = InTarget;
}

void DebugDraw::BuildGeometry(CameraNode* InCamera, uint32_t InTargetHeight) {
    m_Vertices.Clear();
    m_Indices.Clear();

    const Vec3 eye = InCamera->GetPosition();
    const Vec3 viewDirection = InCamera->GetForwardVector();
    const bool orthographic = InCamera->GetProjectionType() == CameraNode::ProjectionType::Orthographic;
    const float targetHeight = (float)glm::max(InTargetHeight, 1u);
    const float orthoWorldPerPixel = InCamera->GetOrthographicSize() / targetHeight;
    const float perspectiveWorldPerPixel =
        2.0f * std::tan(glm::radians(InCamera->GetPerspectiveVerticalFOV()) * 0.5f) / targetHeight;

    for (const DebugLine& line : m_Lines) {
        Vec3 tangent = line.To - line.From;
        if (glm::dot(tangent, tangent) < 1e-12f) {
            continue;
        }
        tangent = glm::normalize(tangent);

        const Vec3 ends[2] = { line.From, line.To };
        Vec3 offsets[2];
        for (int32_t end = 0; end < 2; end++) {
            Vec3 side = glm::cross(tangent, orthographic ? -viewDirection : eye - ends[end]);
            float length = glm::length(side);
            if (length < 1e-6f) {
                const Vec3 reference = std::abs(tangent.y) < 0.9f ? VecUtils::Up : VecUtils::Right;
                side = glm::normalize(glm::cross(tangent, reference));
                length = 1.0f;
            }

            const float depth = glm::max(glm::dot(ends[end] - eye, viewDirection), 1e-3f);
            const float worldPerPixel = orthographic ? orthoWorldPerPixel : perspectiveWorldPerPixel * depth;
            offsets[end] = side * (line.Thickness * 0.5f * worldPerPixel / length);
        }

        const uint32_t base = (uint32_t)m_Vertices.Size();
        m_Vertices.Add(DebugVertex{ line.From - offsets[0], line.LineColor });
        m_Vertices.Add(DebugVertex{ line.From + offsets[0], line.LineColor });
        m_Vertices.Add(DebugVertex{ line.To + offsets[1], line.LineColor });
        m_Vertices.Add(DebugVertex{ line.To - offsets[1], line.LineColor });

        const uint32_t quad[6] = { base, base + 1, base + 2, base, base + 2, base + 3 };
        for (uint32_t index : quad) {
            m_Indices.Add(index);
        }
    }
}

void DebugDraw::Draw(FrameBuffer* InTarget, CameraNode* InCamera) {
    EnsurePipeline(InTarget);
    if (!m_Pipeline.Get()) {
        return;
    }

    BuildGeometry(InCamera, InTarget->GetDesc().Height);
    if (m_Indices.IsEmpty()) {
        return;
    }

    DebugUniformData uniforms;
    uniforms.ViewProjection = InCamera->GetViewProjectionMatrix();
    void* mapped = m_UniformBuffer->MapData(sizeof(uniforms), 0);
    memcpy(mapped, &uniforms, sizeof(uniforms));
    m_UniformBuffer->UnmapData();

    m_VertexBuffer->Update(&m_Vertices[0], (uint32_t)(m_Vertices.Size() * sizeof(DebugVertex)), m_Indices);
    m_Pipeline->Bind();
    m_VertexBuffer->Draw();
}

void DebugDraw::Render(World* InWorld, FrameBuffer* InTarget, CameraNode* InCamera) {
    DebugDraw* debug = Resolve(InWorld);
    if (!debug || !InTarget || !InCamera || debug->m_Lines.IsEmpty()) {
        return;
    }
    debug->Draw(InTarget, InCamera);
}

#else

void DebugDraw::Render(World*, FrameBuffer*, CameraNode*) { }

#endif

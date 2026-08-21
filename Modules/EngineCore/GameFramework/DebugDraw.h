#pragma once
#include "WorldSubsystem.h"
#include "Common/Array.h"
#include "Common/Types.h"
#include "Object/Pointer.h"
#include "DebugDraw.gen.h"

class CameraNode;
class FrameBuffer;
class Pipeline;
class UniformBuffer;
class VertexBuffer;

struct DebugLine {
    Vec3 From;
    Vec3 To;
    Color LineColor;
    float Thickness = 1.0f;
    float Remaining = 0.0f;
};

struct DebugVertex {
    Vec3 Position;
    Color VertexColor;
};

class DebugDraw : public WorldSubsystem {
public:
    ARTIFACT_CLASS();

    static void Line(World* InWorld, const Vec3& InFrom, const Vec3& InTo, const Color& InColor, float InThickness = 1.0f, float InDuration = 0.0f);
    static void Box(World* InWorld, const Vec3& InCenter, const Vec3& InHalfExtents, const Quat& InRotation, const Color& InColor, float InThickness = 1.0f, float InDuration = 0.0f);
    static void Sphere(World* InWorld, const Vec3& InCenter, float InRadius, const Color& InColor, float InThickness = 1.0f, float InDuration = 0.0f);
    static void Capsule(World* InWorld, const Vec3& InCenter, float InHalfHeight, float InRadius, const Quat& InRotation, const Color& InColor, float InThickness = 1.0f, float InDuration = 0.0f);
    static void Cylinder(World* InWorld, const Vec3& InCenter, float InHalfHeight, float InRadius, const Quat& InRotation, const Color& InColor, float InThickness = 1.0f, float InDuration = 0.0f);

    static void Render(World* InWorld, FrameBuffer* InTarget, CameraNode* InCamera);
    static void AdvanceFrame(float InDeltaTime);

    virtual void Initialize() override;
    virtual void Shutdown() override;

private:
    static DebugDraw* Resolve(World* InWorld);

    void Push(const Vec3& InFrom, const Vec3& InTo, const Color& InColor, float InThickness, float InDuration);
    void PushArc(const Vec3& InCenter, const Vec3& InAxisU, const Vec3& InAxisV, float InRadius, float InFromAngle, float InToAngle, int32_t InSegments, const Color& InColor, float InThickness, float InDuration);
    void PushTube(const Vec3& InTop, const Vec3& InBottom, const Vec3& InAxisU, const Vec3& InAxisV, float InRadius, const Color& InColor, float InThickness, float InDuration);
    void Expire(float InDeltaTime);

    void EnsurePipeline(FrameBuffer* InTarget);
    void BuildGeometry(CameraNode* InCamera, uint32_t InTargetHeight);
    void Draw(FrameBuffer* InTarget, CameraNode* InCamera);

    Array<DebugLine> m_Lines;
    Array<DebugVertex> m_Vertices;
    Array<uint32_t> m_Indices;
    SharedObjectPtr<Pipeline> m_Pipeline;
    SharedObjectPtr<UniformBuffer> m_UniformBuffer;
    SharedObjectPtr<VertexBuffer> m_VertexBuffer;
    WeakObjectPtr<FrameBuffer> m_PipelineTarget;
};

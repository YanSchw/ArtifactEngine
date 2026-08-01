#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "Common/Array.h"
#include "Common/Types.h"
#include "GizmoRenderer.gen.h"

class Mesh;
class Pipeline;
class FrameBuffer;
class UniformBuffer;
class ShaderData;
class VertexBuffer;
class CameraNode;
class GizmoGeometry;

struct GizmoDraw {
    Mesh* MeshPtr = nullptr;
    Mat4 Transform = Mat4(1.0f);
    Vec4 Color = Vec4(1.0f);
    uint32_t NodeId = 0;
};

class GizmoRenderer : public Object {
public:
    ARTIFACT_CLASS();

    GizmoRenderer();

    void Render(FrameBuffer* InTarget, CameraNode* InViewCamera, const Array<GizmoDraw>& InDraws);
    void RenderOverlay(FrameBuffer* InTarget, CameraNode* InViewCamera, const GizmoGeometry& InGeometry);

private:
    void UpdateViewProjection(CameraNode* InViewCamera);
    void EnsurePipeline(FrameBuffer* InTarget);
    void EnsureOverlayPipeline(FrameBuffer* InTarget);

    SharedObjectPtr<Pipeline> m_Pipeline;
    SharedObjectPtr<UniformBuffer> m_UniformBuffer;
    Array<SharedObjectPtr<ShaderData>> m_ShaderData;
    WeakObjectPtr<FrameBuffer> m_PipelineTarget;

    SharedObjectPtr<Pipeline> m_OverlayPipeline;
    SharedObjectPtr<VertexBuffer> m_OverlayBuffer;
    WeakObjectPtr<FrameBuffer> m_OverlayTarget;
};

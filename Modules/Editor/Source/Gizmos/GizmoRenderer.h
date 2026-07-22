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
class CameraNode;

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

private:
    void EnsurePipeline(FrameBuffer* InTarget);

    SharedObjectPtr<Pipeline> m_Pipeline;
    SharedObjectPtr<UniformBuffer> m_UniformBuffer;
    Array<SharedObjectPtr<ShaderData>> m_ShaderData;
    WeakObjectPtr<FrameBuffer> m_PipelineTarget;
};

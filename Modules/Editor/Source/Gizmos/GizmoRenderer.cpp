#include "GizmoRenderer.h"
#include "Assets/Mesh.h"
#include "Core/EngineConfig.h"
#include "GameFramework/CameraNode.h"
#include "Platform/FileIO.h"
#include "Rendering/Buffer.h"
#include "Rendering/FrameBuffer.h"
#include "Rendering/Pipeline.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/Shader.h"
#include "Rendering/ShaderData.h"
#include "Rendering/VertexBuffer.h"

static SharedObjectPtr<Shader> s_GizmoShader;

struct GizmoUniformData {
    Mat4 ViewProjection;
};

/** Mirrors the push-constant block in Gizmo.glsl. */
struct GizmoPushData {
    Mat4 WorldTransform = Mat4(1.0f);
    Vec4 Color = Vec4(1.0f);
    uint32_t NodeId = 0;
    uint32_t Padding[3] = { 0, 0, 0 };
};

GizmoRenderer::GizmoRenderer() {
    if (!s_GizmoShader.Get()) {
        s_GizmoShader = Shader::Create(FileIO::ReadFileToString(
            EngineConfig::GetContentDir("Editor") + "/Shaders/Gizmo.glsl"));
    }
    m_UniformBuffer = UniformBuffer::Create(0, sizeof(GizmoUniformData));
}

void GizmoRenderer::EnsurePipeline(FrameBuffer* InTarget) {
    if (m_Pipeline.Get() && m_PipelineTarget.Get() == InTarget) {
        return;
    }
    if (m_Pipeline.Get()) {
        RenderingAPI::GetInstance()->WaitIdle();
    }
    PipelineDesc desc;
    desc.Target = InTarget;
    desc.Shader = s_GizmoShader;
    desc.ClockwiseFrontFace = true;
    desc.Buffers.Add(m_UniformBuffer);
    m_Pipeline = Pipeline::Create(desc);
    m_PipelineTarget = InTarget;
}

void GizmoRenderer::Render(FrameBuffer* InTarget, CameraNode* InViewCamera, const Array<GizmoDraw>& InDraws) {
    if (!InTarget || !InViewCamera || InDraws.IsEmpty() || !s_GizmoShader.Get()) {
        return;
    }
    EnsurePipeline(InTarget);

    GizmoUniformData uniforms;
    uniforms.ViewProjection = InViewCamera->GetViewProjectionMatrix();
    void* mapped = m_UniformBuffer->MapData(sizeof(uniforms), 0);
    memcpy(mapped, &uniforms, sizeof(uniforms));
    m_UniformBuffer->UnmapData();

    m_Pipeline->Bind();

    // The queue records later, so every draw needs shader data that outlives this call.
    while (m_ShaderData.Size() < InDraws.Size()) {
        m_ShaderData.Add(SharedObjectPtr<ShaderData>(new ShaderData()));
    }

    for (int32_t i = 0; i < InDraws.Size(); i++) {
        const GizmoDraw& draw = InDraws[i];
        if (!draw.MeshPtr || !draw.MeshPtr->GetVertexBuffer()) {
            continue;
        }

        GizmoPushData push;
        push.WorldTransform = draw.Transform;
        push.Color = draw.Color;
        push.NodeId = draw.NodeId;
        m_ShaderData[i]->Set(push);

        RenderingAPI::GetInstance()->GetRenderQueue().Push(RenderCommandType::SetShaderData,
            CmdSetShaderData{ m_ShaderData[i].Get() });
        draw.MeshPtr->GetVertexBuffer()->Draw();
    }
}

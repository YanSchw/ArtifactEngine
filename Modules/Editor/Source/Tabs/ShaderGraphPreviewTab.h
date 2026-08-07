#pragma once
#include "MinorTab.h"
#include "Object/Pointer.h"
#include "ShaderGraphPreviewTab.gen.h"

class ShaderGraph;
class FrameBuffer;
class Pipeline;
class UniformBuffer;
class VertexBuffer;
class RenderTargetTexture;
class UIImage;
class Sampler;
class VectorImage;

class ShaderGraphPreviewTab : public MinorTab {
public:
    ARTIFACT_CLASS();

    ShaderGraphPreviewTab();

    virtual String GetTabTitle() const override { return "Preview"; }
    virtual VectorImage* GetTabIcon() const override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;

    void SetShaderGraph(ShaderGraph* InShaderGraph);
    void InvalidatePipeline();

private:
    void EnsureTarget(uint32_t InWidth, uint32_t InHeight);
    void EnsurePipeline();
    void UpdateSceneBuffer(float InDeltaTime);

    WeakObjectPtr<ShaderGraph> m_ShaderGraph;

    SharedObjectPtr<FrameBuffer> m_Target;
    SharedObjectPtr<Pipeline> m_Pipeline;
    SharedObjectPtr<UniformBuffer> m_SceneBuffer;
    SharedObjectPtr<VertexBuffer> m_Mesh;
    SharedObjectPtr<RenderTargetTexture> m_Texture;
    SharedObjectPtr<Sampler> m_Sampler;
    SharedObjectPtr<class ShaderData> m_ShaderData;
    UIImage* m_Image = nullptr;

    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    float m_Time = 0.0f;
    bool m_PipelineDirty = true;
};

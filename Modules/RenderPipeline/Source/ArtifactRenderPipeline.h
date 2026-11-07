#pragma once
#include "Rendering/RenderPipeline.h"
#include "Rendering/FullscreenQuad.h"
#include "Rendering/Image.h"
#include "Object/Pointer.h"
#include "GameFramework/DirectionalLightNode.h"
#include "ArtifactRenderPipeline.gen.h"

class FrameBuffer;
class Sampler;
class Shader;
class UniformBuffer;

class ArtifactRenderPipeline : public RenderPipeline {
public:
    ARTIFACT_CLASS();

    static constexpr int32_t NodeIdAttachment = 1;
    static constexpr SampleCount SceneSampleCount = SampleCount::X8;

    virtual uint32_t PickNodeId(uint32_t InX, uint32_t InY) const override;

protected:
    virtual void Initialize() override;
    virtual void PrepareFrame(RenderContext& InOutContext) override;
    virtual void TransitionRenderStage(RenderStage* InFrom, RenderStage& InTo, const RenderContext& InContext) override;

private:
    void CreateTargets(uint32_t InWidth, uint32_t InHeight);
    DirectionalLightNode* FindSunLight(World* InWorld) const;
    void UploadSceneUniforms(const RenderContext& InContext, DirectionalLightNode* InSun);
    void ResolveGBuffer(RenderStage& InGeometry, RenderStage& InLighting, const RenderContext& InContext);

    RenderStage* m_GeometryStage = nullptr;
    RenderStage* m_LightingStage = nullptr;
    RenderStage* m_ForwardStage = nullptr;

    SharedObjectPtr<UniformBuffer> m_SceneUniforms;
    SharedObjectPtr<Shader> m_ResolveShader;
    SharedObjectPtr<Sampler> m_GBufferSampler;
    FullscreenQuad m_ResolveQuad;
    ShadowMapPlaceholder m_ShadowPlaceholder;
    WeakObjectPtr<DirectionalLightNode> m_ShadowSource;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    float m_Time = 0.0f;
};

#pragma once
#include "Rendering/RenderPipeline.h"
#include "Rendering/Image.h"
#include "Object/Pointer.h"
#include "Common/Map.h"
#include "Common/UUID.h"
#include "GameFramework/DirectionalLightNode.h"
#include "ArtifactRenderPipeline.gen.h"

class Material;
class Sampler;
class UniformBuffer;
class Pipeline;
class FrameBuffer;
class CameraNode;

class ArtifactRenderPipeline : public RenderPipeline {
public:
    ARTIFACT_CLASS();

    static constexpr int32_t NodeIdAttachment = 1;
    static constexpr SampleCount SceneSampleCount = SampleCount::X8;

    ArtifactRenderPipeline();
    ~ArtifactRenderPipeline();

    void Invalidate(uint32_t InWidth, uint32_t InHeight);

    virtual void Render(double InDeltaTime, const RenderParams& InParams) override;
    virtual SharedObjectPtr<class ImageView> GetFinalImageView() const override;
    virtual SharedObjectPtr<class FrameBuffer> GetFrameBuffer() const override { return m_FrameBuffer; }
    virtual uint32_t PickNodeId(uint32_t InX, uint32_t InY) const override;

private:
    /** A material's pipeline, rebuilt whenever the resources it was created from are replaced. */
    struct MaterialPipeline {
        SharedObjectPtr<Pipeline> Instance;
        Array<void*> Resources;
    };

    CameraNode* ResolveCamera(const RenderParams& InParams) const;
    DirectionalLightNode* FindSunLight(const RenderParams& InParams) const;
    void UpdateUniformData(double InDeltaTime, CameraNode* InCamera, DirectionalLightNode* InSun,
                           const RenderParams& InParams);
    Pipeline* ResolvePipeline(Material* InMaterial);

    SharedObjectPtr<UniformBuffer> m_UniformBuffer;
    SharedObjectPtr<FrameBuffer> m_FrameBuffer;
    SharedObjectPtr<Sampler> m_Sampler;
    Map<UUID, MaterialPipeline> m_MaterialPipelines;
    ShadowMapPlaceholder m_ShadowPlaceholder;
    WeakObjectPtr<DirectionalLightNode> m_ShadowSource;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    float m_Time = 0.0f;
};

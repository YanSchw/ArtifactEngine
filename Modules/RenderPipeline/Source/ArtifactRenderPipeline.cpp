#include "ArtifactRenderPipeline.h"
#include "Passes/DebugDrawPass.h"
#include "Passes/DeferredLightPass.h"
#include "Passes/StaticMeshPass.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/Sampler.h"
#include "Rendering/Buffer.h"
#include "Rendering/FrameBuffer.h"
#include "Rendering/GBuffer.h"
#include "Rendering/Image.h"
#include "Rendering/SceneUniforms.h"
#include "Rendering/Shader.h"
#include "Rendering/ShaderLibrary.h"
#include "GameFramework/World.h"
#include "GameFramework/CameraNode.h"
#include "GameFramework/PointLightNode.h"

static const Vec4 s_BackgroundColor = Vec4(0.08f, 0.09f, 0.11f, 1.0f);

static SharedObjectPtr<ImageView> CreateAttachment(uint32_t InWidth, uint32_t InHeight, ImageFormat InFormat, ImageUsage InUsage) {
    ImageDesc imageDesc;
    imageDesc.Width = InWidth;
    imageDesc.Height = InHeight;
    imageDesc.Format = InFormat;
    imageDesc.Usage = InUsage;

    ImageViewDesc viewDesc;
    viewDesc.ImagePtr = Image::Create(imageDesc);
    viewDesc.Format = InFormat;
    viewDesc.Aspect = (InUsage & ImageUsage::DepthStencil) != ImageUsage::None ? ImageAspect::Depth : ImageAspect::Color;
    return ImageView::Create(viewDesc);
}

void ArtifactRenderPipeline::Initialize() {
    m_GeometryStage = SetFirstStage(RenderStageName::DeferredGeometry);
    m_LightingStage = m_GeometryStage->Then(RenderStageName::DeferredLighting);
    m_ForwardStage = m_LightingStage->Then(RenderStageName::Forward);

    m_GeometryStage->AddPass<StaticMeshDeferredPass>();
    m_LightingStage->AddPass<DeferredDirectionalLightPass>();
    m_LightingStage->AddPass<DeferredPointLightPass>();
    m_ForwardStage->AddPass<StaticMeshForwardPass>();
    m_ForwardStage->AddPass<DebugDrawPass>();

    m_SceneUniforms = UniformBuffer::Create(0, sizeof(SceneUniformData));
    m_ResolveShader = ShaderLibrary::CreateShader("/Shaders/Deferred/Resolve.glsl");
    m_GBufferSampler = Sampler::Create({ FilterMode::Nearest, FilterMode::Nearest,
                                         AddressMode::Clamp, AddressMode::Clamp, AddressMode::Clamp });
    m_ShadowPlaceholder.Create();
    CreateTargets(100, 100);
}

void ArtifactRenderPipeline::CreateTargets(uint32_t InWidth, uint32_t InHeight) {
    if (m_GeometryStage->GetTarget()) {
        RenderingAPI::GetInstance()->WaitIdle();
    }
    m_Width = InWidth;
    m_Height = InHeight;

    FrameBufferDesc gbufferDesc;
    gbufferDesc.Width = InWidth;
    gbufferDesc.Height = InHeight;
    for (int32_t attachment = 0; attachment < GBufferLayout::AttachmentCount; attachment++) {
        gbufferDesc.ColorAttachments.Add(CreateAttachment(InWidth, InHeight, GBufferLayout::Format,
                                                          ImageUsage::ColorAttachment | ImageUsage::Sampled));
    }
    gbufferDesc.DepthAttachment = CreateAttachment(InWidth, InHeight, GBufferLayout::DepthFormat,
                                                   ImageUsage::DepthStencil | ImageUsage::Sampled);
    gbufferDesc.ClearColor = Vec4(0.0f);
    m_GeometryStage->SetTarget(FrameBuffer::Create(gbufferDesc));

    FrameBufferDesc sceneDesc;
    sceneDesc.Width = InWidth;
    sceneDesc.Height = InHeight;
    sceneDesc.Samples = SceneSampleCount;
    sceneDesc.ColorAttachments.Add(CreateAttachment(InWidth, InHeight, ImageFormat::RGBA8,
                                                    ImageUsage::ColorAttachment | ImageUsage::Sampled));
    // The id is packed into RGBA8
    sceneDesc.ColorAttachments.Add(CreateAttachment(InWidth, InHeight, ImageFormat::RGBA8,
                                                    ImageUsage::ColorAttachment | ImageUsage::TransferSrc | ImageUsage::Sampled));
    sceneDesc.DepthAttachment = CreateAttachment(InWidth, InHeight, ImageFormat::Depth32F, ImageUsage::DepthStencil);
    sceneDesc.ClearColor = s_BackgroundColor;
    sceneDesc.ClearColors.Add(s_BackgroundColor);
    sceneDesc.ClearColors.Add(Vec4(0.0f));
    SharedObjectPtr<FrameBuffer> scene = FrameBuffer::Create(sceneDesc);
    m_LightingStage->SetTarget(scene);
    m_ForwardStage->SetTarget(scene);
}

void ArtifactRenderPipeline::PrepareFrame(RenderContext& InOutContext) {
    if (m_Width != InOutContext.Width || m_Height != InOutContext.Height) {
        CreateTargets(InOutContext.Width, InOutContext.Height);
    }

    CameraNode* camera = InOutContext.Camera;
    if (camera) {
        camera->SetAspectRatio(InOutContext.Width / (float)InOutContext.Height);
    }

    DirectionalLightNode* sun = FindSunLight(InOutContext.World);
    const bool shadowsRendered = sun && camera && sun->RenderShadowMaps(*InOutContext.World, *camera);
    if (!shadowsRendered) {
        m_ShadowPlaceholder.Clear();
    }
    m_ShadowSource = shadowsRendered ? sun : nullptr;

    InOutContext.SceneUniforms = m_SceneUniforms.Get();
    InOutContext.ShadowMap = shadowsRendered ? sun->GetShadowMapView() : m_ShadowPlaceholder.GetView();
    InOutContext.ShadowSampler = shadowsRendered ? sun->GetShadowSampler() : m_ShadowPlaceholder.GetSampler();
    UploadSceneUniforms(InOutContext, sun);
}

void ArtifactRenderPipeline::TransitionRenderStage(RenderStage* InFrom, RenderStage& InTo, const RenderContext& InContext) {
    if (InFrom && InFrom->GetTarget() == InTo.GetTarget()) {
        return;
    }
    InTo.BeginTarget();
    if (InFrom == m_GeometryStage && &InTo == m_LightingStage) {
        ResolveGBuffer(*InFrom, InTo, InContext);
    }
}

/** Carries depth, node ids and emissive over, so lighting only adds onto it and forward geometry
 *  is occluded by deferred geometry. */
void ArtifactRenderPipeline::ResolveGBuffer(RenderStage& InGeometry, RenderStage& InLighting, const RenderContext& InContext) {
    if (!m_ResolveShader) {
        return;
    }
    m_ResolveQuad.Draw(DeferredLightPass::DescribeGBufferRead(*InLighting.GetTarget(), *InGeometry.GetTarget(),
                                                              *m_ResolveShader, *m_GBufferSampler, InContext));
}

DirectionalLightNode* ArtifactRenderPipeline::FindSunLight(World* InWorld) const {
    if (!InWorld) {
        return nullptr;
    }
    for (Node* node : InWorld->GetAllNodes()) {
        DirectionalLightNode* light = node->As<DirectionalLightNode>();
        if (light && light->IsEnabled()) {
            return light;
        }
    }
    return nullptr;
}

void ArtifactRenderPipeline::UploadSceneUniforms(const RenderContext& InContext, DirectionalLightNode* InSun) {
    m_Time += (float)InContext.DeltaTime;

    SceneUniformData data;
    data.Time = m_Time;
    if (InContext.Camera) {
        data.ViewProjection = InContext.Camera->GetViewProjectionMatrix();
        data.InverseViewProjection = glm::inverse(data.ViewProjection);
        data.CameraPosition = Vec4(InContext.Camera->GetPosition(), 1.0f);
    }

    if (InSun) {
        data.SunDirection = Vec4(InSun->GetDirection(), 0.0f);
        data.SunColor = InSun->GetColor() * InSun->GetIntensity();
        data.AmbientColor = InSun->GetAmbientColor() * InSun->GetAmbientIntensity();
        data.CascadeTexelSizes = InSun->GetCascadeTexelSizes();
        data.ShadowParams = m_ShadowSource.Get() == InSun ? InSun->GetShadowParams() : Vec4(0.0f);
        for (int32_t cascade = 0; cascade < SceneUniformData::ShadowCascadeCount; cascade++) {
            data.ShadowMatrices[cascade] = InSun->GetCascadeMatrix(cascade);
        }
    }

    if (InContext.World) {
        for (Node* node : InContext.World->GetAllNodes()) {
            PointLightNode* light = node->As<PointLightNode>();
            if (!light || !light->IsEnabled() || data.PointLightCount >= SceneUniformData::MaxPointLights) {
                continue;
            }
            const int32_t index = (int32_t)data.PointLightCount;
            data.PointLightPositions[index] = Vec4(light->GetPosition(), light->GetRadius());
            data.PointLightColors[index] = light->GetColor() * light->GetIntensity();
            data.PointLightCount += 1.0f;
        }
    }

    void* mapped = m_SceneUniforms->MapData(sizeof(data), 0);
    memcpy(mapped, &data, sizeof(data));
    m_SceneUniforms->UnmapData();
}

uint32_t ArtifactRenderPipeline::PickNodeId(uint32_t InX, uint32_t InY) const {
    FrameBuffer* scene = GetFrameBuffer().Get();
    return scene ? scene->ReadPixelUint(NodeIdAttachment, InX, InY) : 0;
}

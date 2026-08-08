#include "DirectionalLightNode.h"

#include "Assets/Mesh.h"
#include "CameraNode.h"
#include "StaticMeshNode.h"
#include "World.h"
#include "Rendering/Buffer.h"
#include "Rendering/FrameBuffer.h"
#include "Rendering/Image.h"
#include "Rendering/Pipeline.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/RenderingCommand.h"
#include "Rendering/Sampler.h"
#include "Rendering/ShaderLibrary.h"
#include "Rendering/VertexBuffer.h"

static SharedObjectPtr<Sampler> CreateShadowSampler() {
    SamplerDesc samplerDesc;
    samplerDesc.MinFilter = FilterMode::Linear;
    samplerDesc.MagFilter = FilterMode::Linear;
    samplerDesc.AddressU = AddressMode::Clamp;
    samplerDesc.AddressV = AddressMode::Clamp;
    samplerDesc.AddressW = AddressMode::Clamp;
    samplerDesc.Compare = CompareOp::LessOrEqual;
    return Sampler::Create(samplerDesc);
}

static SharedObjectPtr<Image> CreateShadowImage(uint32_t InResolution) {
    ImageDesc imageDesc;
    imageDesc.Width = InResolution;
    imageDesc.Height = InResolution;
    imageDesc.ArrayLayers = DirectionalLightNode::CascadeCount;
    imageDesc.Format = ImageFormat::Depth32F;
    imageDesc.Usage = ImageUsage::DepthStencil | ImageUsage::Sampled;
    return Image::Create(imageDesc);
}

static SharedObjectPtr<ImageView> CreateShadowView(const SharedObjectPtr<Image>& InImage, ImageViewType InType,
                                                   uint32_t InBaseLayer, uint32_t InLayerCount) {
    ImageViewDesc viewDesc;
    viewDesc.ImagePtr = InImage;
    viewDesc.ViewType = InType;
    viewDesc.Format = ImageFormat::Depth32F;
    viewDesc.Aspect = ImageAspect::Depth;
    viewDesc.BaseLayer = InBaseLayer;
    viewDesc.LayerCount = InLayerCount;
    return ImageView::Create(viewDesc);
}

void ShadowMapPlaceholder::Create() {
    m_Image = CreateShadowImage(1);
    m_View = CreateShadowView(m_Image, ImageViewType::Type2DArray, 0, DirectionalLightNode::CascadeCount);
    m_Sampler = CreateShadowSampler();

    FrameBufferDesc targetDesc;
    targetDesc.Width = 1;
    targetDesc.Height = 1;
    targetDesc.DepthAttachment = m_View;
    m_Target = FrameBuffer::Create(targetDesc);
}

void ShadowMapPlaceholder::Clear() const {
    if (!m_Target.Get()) {
        return;
    }
    RenderingAPI::GetInstance()->GetRenderQueue().Push(RenderCommandType::BeginRenderPass,
                                                       CmdBeginRenderPass{ m_Target });
}

ImageView* ShadowMapPlaceholder::GetView() const {
    return m_View.Get();
}

Sampler* ShadowMapPlaceholder::GetSampler() const {
    return m_Sampler.Get();
}

DirectionalLightNode::DirectionalLightNode() {
    m_CascadeMatrices.Resize(CascadeCount);
    for (Mat4& matrix : m_CascadeMatrices) {
        matrix = Mat4(1.0f);
    }
    SetLocalEulerRotation(Vec3(50.0f, -35.0f, 0.0f));
}

DirectionalLightNode::~DirectionalLightNode() {
    ReleaseShadowMaps();
}

void DirectionalLightNode::SetCastShadows(bool InCastShadows) {
    m_CastShadows = InCastShadows;
    if (!m_CastShadows) {
        ReleaseShadowMaps();
    }
}

ImageView* DirectionalLightNode::GetShadowMapView() const {
    return m_ShadowArrayView.Get();
}

Sampler* DirectionalLightNode::GetShadowSampler() const {
    return m_ShadowSampler.Get();
}

const Mat4& DirectionalLightNode::GetCascadeMatrix(int32_t InCascade) const {
    return m_CascadeMatrices[glm::clamp(InCascade, 0, CascadeCount - 1)];
}

Vec4 DirectionalLightNode::GetShadowParams() const {
    if (!m_ShadowArrayView.Get()) {
        return Vec4(0.0f);
    }
    const float filterStep = glm::max(m_Softness, 0.0f) / (float)m_ShadowMapsResolution;
    return Vec4(m_DepthBias, m_NormalBias, (float)CascadeCount, filterStep);
}

uint32_t DirectionalLightNode::GetClampedResolution() const {
    return glm::clamp(m_ShadowMapResolution, 128u, 8192u);
}

bool DirectionalLightNode::EnsureShadowMaps() {
    if (!RenderingAPI::GetInstance()) {
        return false;
    }

    if (!m_ShadowShader.Get()) {
        m_ShadowShader = ShaderLibrary::CreateShader(ShadowShaderKey);
        if (!m_ShadowShader.Get()) {
            return false;
        }
    }

    const uint32_t resolution = GetClampedResolution();
    if (m_ShadowArrayView.Get() && m_ShadowMapsResolution == resolution) {
        return true;
    }

    ReleaseShadowMaps();

    m_ShadowImage = CreateShadowImage(resolution);
    m_ShadowArrayView = CreateShadowView(m_ShadowImage, ImageViewType::Type2DArray, 0, CascadeCount);
    m_ShadowSampler = CreateShadowSampler();

    for (int32_t cascade = 0; cascade < CascadeCount; cascade++) {
        m_CascadeViews.Add(CreateShadowView(m_ShadowImage, ImageViewType::Type2D, (uint32_t)cascade, 1));

        FrameBufferDesc targetDesc;
        targetDesc.Width = resolution;
        targetDesc.Height = resolution;
        targetDesc.DepthAttachment = m_CascadeViews[cascade];
        m_CascadeTargets.Add(FrameBuffer::Create(targetDesc));

        m_CascadeBuffers.Add(UniformBuffer::Create(0, sizeof(Mat4)));

        PipelineDesc pipelineDesc;
        pipelineDesc.Target = m_CascadeTargets[cascade];
        pipelineDesc.Shader = m_ShadowShader;
        pipelineDesc.Buffers.Add(m_CascadeBuffers[cascade]);
        m_CascadePipelines.Add(Pipeline::Create(pipelineDesc));
    }

    m_ShadowMapsResolution = resolution;
    return true;
}

void DirectionalLightNode::ReleaseShadowMaps() {
    if (!m_ShadowImage.Get()) {
        return;
    }
    RenderingAPI::GetInstance()->WaitIdle();

    m_CascadePipelines.Clear();
    m_CascadeBuffers.Clear();
    m_CascadeTargets.Clear();
    m_CascadeViews.Clear();
    m_ShadowSampler = nullptr;
    m_ShadowArrayView = nullptr;
    m_ShadowImage = nullptr;
    m_ShadowMapsResolution = 0;
}

void DirectionalLightNode::UpdateCascades(const CameraNode& InCamera) {
    const Mat4 cameraTransform = InCamera.GetTransformMatrix();
    const Vec3 cameraPosition = Vec3(cameraTransform[3]);
    const Vec3 cameraRight = glm::normalize(Vec3(cameraTransform[0]));
    const Vec3 cameraUp = glm::normalize(Vec3(cameraTransform[1]));
    const Vec3 cameraForward = glm::normalize(Vec3(cameraTransform[2]));

    const float tanHalfHeight = glm::tan(glm::radians(InCamera.GetPerspectiveVerticalFOV()) * 0.5f);
    const float tanHalfWidth = tanHalfHeight * InCamera.GetAspectRatio();

    const float nearClip = glm::max(InCamera.GetPerspectiveNearClip(), 0.01f);
    const float farClip = glm::max(glm::min(InCamera.GetPerspectiveFarClip(), nearClip + m_ShadowDistance),
                                   nearClip + 1.0f);

    const Vec3 direction = GetDirection();
    const Vec3 lightUp = glm::abs(direction.y) > 0.99f ? VecUtils::Forward : VecUtils::Up;
    const Mat4 lightView = glm::lookAtLH(Vec3(0.0f), direction, lightUp);
    const float resolution = (float)m_ShadowMapsResolution;

    float splitNear = nearClip;
    for (int32_t cascade = 0; cascade < CascadeCount; cascade++) {
        const float fraction = (float)(cascade + 1) / (float)CascadeCount;
        const float splitFar = glm::mix(nearClip + (farClip - nearClip) * fraction,
                                        nearClip * glm::pow(farClip / nearClip, fraction),
                                        glm::clamp(m_CascadeDistribution, 0.0f, 1.0f));

        Vec3 corners[8];
        for (int32_t i = 0; i < 8; i++) {
            const float distance = (i & 4) ? splitFar : splitNear;
            corners[i] = cameraPosition + cameraForward * distance
                       + cameraRight * (distance * tanHalfWidth * ((i & 1) ? 1.0f : -1.0f))
                       + cameraUp * (distance * tanHalfHeight * ((i & 2) ? 1.0f : -1.0f));
        }

        Vec3 center = Vec3(0.0f);
        for (const Vec3& corner : corners) {
            center += corner;
        }
        center /= 8.0f;

        float radius = 0.0f;
        for (const Vec3& corner : corners) {
            radius = glm::max(radius, glm::length(corner - center));
        }
        // Quantizing the radius keeps the fit stable while the camera turns, which is what stops
        // the shadow edges from crawling along their casters.
        radius = glm::ceil(radius * 16.0f) / 16.0f;

        const float texelSize = (radius * 2.0f) / resolution;
        Vec3 lightSpaceCenter = Vec3(lightView * Vec4(center, 1.0f));
        lightSpaceCenter.x = glm::floor(lightSpaceCenter.x / texelSize) * texelSize;
        lightSpaceCenter.y = glm::floor(lightSpaceCenter.y / texelSize) * texelSize;

        const Mat4 projection = glm::orthoLH(
            lightSpaceCenter.x - radius, lightSpaceCenter.x + radius,
            lightSpaceCenter.y - radius, lightSpaceCenter.y + radius,
            lightSpaceCenter.z - radius - m_CasterDistance, lightSpaceCenter.z + radius);

        m_CascadeMatrices[cascade] = projection * lightView;
        m_CascadeTexelSizes[cascade] = texelSize;
        splitNear = splitFar;
    }
}

void DirectionalLightNode::RecordCascadePass(World& InWorld, int32_t InCascade) {
    RenderCommandQueue& renderQueue = RenderingAPI::GetInstance()->GetRenderQueue();
    renderQueue.Push(RenderCommandType::BeginRenderPass, CmdBeginRenderPass{ m_CascadeTargets[InCascade] });

    for (Node* node : InWorld.GetAllNodes()) {
        StaticMeshNode* staticMesh = node->As<StaticMeshNode>();
        if (!staticMesh || !staticMesh->IsEnabled() || !staticMesh->GetCastShadow()) {
            continue;
        }

        Mesh* mesh = staticMesh->GetMesh();
        VertexBuffer* vertexBuffer = mesh ? mesh->GetVertexBuffer() : nullptr;
        if (!vertexBuffer) {
            continue;
        }

        renderQueue.Push(RenderCommandType::BindPipeline, CmdBindPipeline{ m_CascadePipelines[InCascade] });
        renderQueue.Push(RenderCommandType::SetShaderData, CmdSetShaderData{ staticMesh->GetPerMeshShaderData() });
        vertexBuffer->Draw();
    }
}

bool DirectionalLightNode::RenderShadowMaps(World& InWorld, const CameraNode& InCamera) {
    if (!m_CastShadows || !IsEnabled() || !EnsureShadowMaps()) {
        return false;
    }

    UpdateCascades(InCamera);

    for (int32_t cascade = 0; cascade < CascadeCount; cascade++) {
        void* mapped = m_CascadeBuffers[cascade]->MapData(sizeof(Mat4), 0);
        memcpy(mapped, &m_CascadeMatrices[cascade], sizeof(Mat4));
        m_CascadeBuffers[cascade]->UnmapData();

        RecordCascadePass(InWorld, cascade);
    }
    return true;
}

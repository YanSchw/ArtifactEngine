#include "AssetPreviewTab.h"

#include "Assets/AssetManager.h"
#include "Assets/Material.h"
#include "Assets/Mesh.h"
#include "Assets/Texture2D.h"
#include "Rendering/Buffer.h"
#include "Rendering/FrameBuffer.h"
#include "Rendering/Image.h"
#include "Rendering/Pipeline.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/RenderTargetTexture.h"
#include "Rendering/Sampler.h"
#include "Rendering/Texture.h"
#include "Rendering/Shader.h"
#include "Rendering/ShaderData.h"
#include "Rendering/SceneUniforms.h"
#include "Rendering/ShaderTemplate.h"
#include "Rendering/VertexBuffer.h"
#include "GameFramework/UIImage.h"
#include "UI/EditorIcons.h"
#include "UI/EditorStyle.h"
#include "UI/UIContextMenu.h"

#include <algorithm>

static const UUID s_DefaultMesh = UUID::FromString("c6308770-3a5b-4b2b-9cec-14ba803ff817");
static const Vec4 s_ClearColor = HexColor(0x141417);

static const Vec3 s_KeyLightDirection = glm::normalize(Vec3(0.45f, -0.8f, 0.4f));

static const char* const s_PrimitiveMeshes[] = { "Sphere", "Cube", "Quad" };
static constexpr float s_SwayAmplitude = glm::radians(45.0f);
static constexpr float s_SwaySpeed = 0.9f;
static constexpr float s_SpinSpeed = 0.6f;
static constexpr float s_DragSpeed = 0.008f;
static constexpr float s_PitchLimit = 1.4f;

static Mesh* FindMeshNamed(const String& InName) {
    for (Asset* asset : AssetManager::Get().GetAssetsOfClass(Mesh::StaticClass())) {
        Mesh* mesh = Cast<Mesh>(asset);
        if (mesh && mesh->GetDisplayName() == InName) {
            return mesh;
        }
    }
    return nullptr;
}

struct PreviewPushData {
    Mat4 WorldTransform = Mat4(1.0f);
    uint32_t NodeId = 0;
    uint32_t Padding[3] = { 0, 0, 0 };
};

AssetPreviewTab::AssetPreviewTab() {
    Interactable = true;
    m_Texture = Object::Create<RenderTargetTexture>();

    m_Image = Add<UIImage>();
    m_Image->Fill();
    m_Image->Image = m_Texture;

    SamplerDesc samplerDesc;
    samplerDesc.MinFilter = FilterMode::Linear;
    samplerDesc.MagFilter = FilterMode::Linear;
    m_Sampler = Sampler::Create(samplerDesc);

    m_SceneBuffer = UniformBuffer::Create(0, sizeof(SceneUniformData));
    m_ShadowMap.Create();
}

VectorImage* AssetPreviewTab::GetTabIcon() const {
    return EditorIcons::Viewport();
}

void AssetPreviewTab::SetMaterial(Material* InMaterial) {
    m_Material = InMaterial;
    AssetManager::Get().LoadAsset(InMaterial);
    InvalidatePipeline();
}

void AssetPreviewTab::SetMesh(Mesh* InMesh) {
    m_Mesh = InMesh;
    AssetManager::Get().LoadAsset(InMesh);
    InvalidatePipeline();
}

void AssetPreviewTab::InvalidatePipeline() {
    m_PipelineDirty = true;
}

Material* AssetPreviewTab::ResolveMaterial(int32_t InSlot) const {
    if (Material* material = m_Material.Get()) {
        return material;
    }
    Mesh* mesh = ResolveMesh();
    return mesh ? mesh->GetMaterial(InSlot) : Material::GetDefault();
}

Mesh* AssetPreviewTab::ResolveMesh() const {
    Mesh* mesh = m_Mesh.Get();
    return mesh ? mesh : AssetManager::Get().GetAsset<Mesh>(s_DefaultMesh);
}

void AssetPreviewTab::EnsureTarget(uint32_t InWidth, uint32_t InHeight) {
    if (m_Target && m_Width == InWidth && m_Height == InHeight) {
        return;
    }
    if (m_Target) {
        RenderingAPI::GetInstance()->WaitIdle();
    }

    m_Width = InWidth;
    m_Height = InHeight;

    auto makeColor = [&](ImageFormat InFormat) {
        ImageDesc imageDesc;
        imageDesc.Width = InWidth;
        imageDesc.Height = InHeight;
        imageDesc.Format = InFormat;
        imageDesc.Usage = ImageUsage::ColorAttachment | ImageUsage::Sampled;

        ImageViewDesc viewDesc;
        viewDesc.ImagePtr = Image::Create(imageDesc);
        viewDesc.Format = InFormat;
        return ImageView::Create(viewDesc);
    };

    ImageDesc depthDesc;
    depthDesc.Width = InWidth;
    depthDesc.Height = InHeight;
    depthDesc.Format = ImageFormat::Depth32F;
    depthDesc.Usage = ImageUsage::DepthStencil;

    ImageViewDesc depthViewDesc;
    depthViewDesc.ImagePtr = Image::Create(depthDesc);
    depthViewDesc.Format = ImageFormat::Depth32F;
    depthViewDesc.Aspect = ImageAspect::Depth;

    FrameBufferDesc targetDesc;
    targetDesc.Width = InWidth;
    targetDesc.Height = InHeight;
    targetDesc.Samples = SampleCount::X4;
    targetDesc.ColorAttachments.Add(makeColor(ImageFormat::RGBA8));
    targetDesc.ColorAttachments.Add(makeColor(ImageFormat::RGBA8));
    targetDesc.DepthAttachment = ImageView::Create(depthViewDesc);
    targetDesc.ClearColor = s_ClearColor;
    targetDesc.ClearColors.Add(s_ClearColor);
    targetDesc.ClearColors.Add(Vec4(0.0f));

    m_Target = FrameBuffer::Create(targetDesc);
    m_Texture->SetView(m_Target->GetDesc().ColorAttachments[0]);
    m_PipelineDirty = true;
}

Pipeline* AssetPreviewTab::EnsurePipeline(PreviewSlotPipeline& InOutSlot, Material* InMaterial) {
    Shader* shader = InMaterial ? InMaterial->GetShader() : nullptr;
    if (!m_Target || !shader) {
        InOutSlot = PreviewSlotPipeline();
        return nullptr;
    }

    // A pipeline missing a binding the shader declares is a descriptor-set mismatch, so keep the
    // previous one until every texture has streamed in.
    Array<void*> resources = { shader, InMaterial->GetPropertyBuffer() };
    PipelineDesc desc;
    desc.ImageBindings.Add({ ShaderTemplate::ShadowMapBinding, m_ShadowMap.GetView(), m_ShadowMap.GetSampler() });
    for (const MaterialTextureBinding& binding : InMaterial->GetTextureBindings()) {
        AssetManager::Get().LoadAsset(binding.Texture);
        Texture* texture = binding.Texture ? binding.Texture->GetTexture() : nullptr;
        if (!texture) {
            return InOutSlot.Instance.Get();
        }
        resources.Add(texture);
        desc.ImageBindings.Add({ binding.Binding, texture->GetDefaultView(), m_Sampler });
    }

    if (!InOutSlot.Dirty && InOutSlot.Instance && InOutSlot.Resources == resources) {
        return InOutSlot.Instance.Get();
    }

    if (InOutSlot.Instance) {
        RenderingAPI::GetInstance()->WaitIdle();
    }

    desc.Target = m_Target;
    desc.Shader = shader;
    desc.Buffers.Add(m_SceneBuffer);
    if (UniformBuffer* properties = InMaterial->GetPropertyBuffer()) {
        desc.Buffers.Add(properties);
    }

    InOutSlot.Instance = Pipeline::Create(desc);
    InOutSlot.Resources = resources;
    InOutSlot.Dirty = false;
    return InOutSlot.Instance.Get();
}

void AssetPreviewTab::BuildMeshMenu(UIMenuModel& OutMenu) {
    WeakObjectPtr<AssetPreviewTab> self = this;
    const Mesh* current = ResolveMesh();

    for (const char* const name : s_PrimitiveMeshes) {
        WeakObjectPtr<Mesh> mesh = FindMeshNamed(name);
        OutMenu.Item(name, [self, mesh] {
            if (AssetPreviewTab* tab = self.Get()) {
                tab->SetMesh(mesh.Get());
            }
        }).Enabled(mesh.Get() != nullptr).Checked(mesh.Get() && mesh.Get() == current);
    }

    OutMenu.Separator();
    OutMenu.Submenu("Custom Mesh", [self, current](UIMenuModel& OutSub) {
        OutSub.Searchable("Search meshes");
        for (Asset* asset : AssetManager::Get().GetAssetsOfClass(Mesh::StaticClass())) {
            WeakObjectPtr<Mesh> mesh = Cast<Mesh>(asset);
            if (!mesh.Get()) {
                continue;
            }
            OutSub.Item(mesh.Get()->GetDisplayName(), [self, mesh] {
                if (AssetPreviewTab* tab = self.Get()) {
                    tab->SetMesh(mesh.Get());
                }
            }).Checked(mesh.Get() == current);
        }
    });
}

void AssetPreviewTab::BuildPanMenu(UIMenuModel& OutMenu) {
    WeakObjectPtr<AssetPreviewTab> self = this;
    const PreviewPanMode modes[] = { PreviewPanMode::Rotate, PreviewPanMode::PanLeftRight, PreviewPanMode::None };
    const char* const labels[] = { "Rotate", "Pan Left-Right", "None" };

    for (int32_t i = 0; i < 3; i++) {
        const PreviewPanMode mode = modes[i];
        OutMenu.Item(labels[i], [self, mode] {
            if (AssetPreviewTab* tab = self.Get()) {
                tab->m_PanMode = mode;
                tab->m_SwayPhase = 0.0f;
            }
        }).Checked(m_PanMode == mode);
    }
}

void AssetPreviewTab::AdvanceMotion(float InDeltaTime) {
    if (m_PanMode == PreviewPanMode::Rotate) {
        m_Yaw += InDeltaTime * s_SpinSpeed;
    } else if (m_PanMode == PreviewPanMode::PanLeftRight) {
        const float previous = glm::sin(m_SwayPhase);
        m_SwayPhase += InDeltaTime * s_SwaySpeed;
        m_Yaw += (glm::sin(m_SwayPhase) - previous) * s_SwayAmplitude;
    }
}

Mat4 AssetPreviewTab::GetMeshRotation() const {
    return glm::rotate(Mat4(1.0f), m_Pitch, VecUtils::Right)
         * glm::rotate(Mat4(1.0f), m_Yaw, VecUtils::Up);
}

void AssetPreviewTab::OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) {
    (void)InCursorPos;
    m_Yaw -= InDelta.x * s_DragSpeed;
    m_Pitch = glm::clamp(m_Pitch - InDelta.y * s_DragSpeed, -s_PitchLimit, s_PitchLimit);
}

bool AssetPreviewTab::OnSecondaryClick(const Vec2& InCursorPos) {
    WeakObjectPtr<AssetPreviewTab> self = this;

    UIMenuModel menu;
    menu.Section("Preview");
    menu.Submenu("Mesh", [self](UIMenuModel& OutSub) {
        if (AssetPreviewTab* tab = self.Get()) {
            tab->BuildMeshMenu(OutSub);
        }
    });
    menu.Submenu("Panning Mode", [self](UIMenuModel& OutSub) {
        if (AssetPreviewTab* tab = self.Get()) {
            tab->BuildPanMenu(OutSub);
        }
    });

    UIContextMenu::OpenAt(*this, InCursorPos, menu);
    return true;
}

void AssetPreviewTab::UpdateSceneBuffer(float InDeltaTime, const Mesh& InMesh) {
    m_Time += InDeltaTime;

    const float radius = InMesh.GetBoundsRadius();
    const float aspect = m_Height > 0 ? (float)m_Width / (float)m_Height : 1.0f;
    Mat4 projection = glm::perspectiveLH(glm::radians(45.0f), aspect, radius * 0.01f, radius * 50.0f);
    projection[1][1] *= -1.0f;
    const Vec3 eye = Vec3(0.0f, radius * 0.9f, -radius * 3.2f);
    const Mat4 view = glm::lookAtLH(eye, Vec3(0.0f), VecUtils::Up);

    SceneUniformData data;
    data.ViewProjection = projection * view;
    data.CameraPosition = Vec4(eye, 1.0f);
    data.Time = m_Time;
    data.SunDirection = Vec4(s_KeyLightDirection, 0.0f);
    data.SunColor = Vec4(1.15f, 1.13f, 1.06f, 0.0f);
    for (Color& ambient : data.AmbientCube) {
        ambient = Color(0.16f, 0.19f, 0.23f, 0.0f);
    }

    void* mapped = m_SceneBuffer->MapData(sizeof(data), 0);
    memcpy(mapped, &data, sizeof(data));
    m_SceneBuffer->UnmapData();
}

void AssetPreviewTab::OnUIUpdate(const UIFrameContext& InContext) {
    MinorTab::OnUIUpdate(InContext);

    const UIRectF rect = GetGeometry();
    const uint32_t width = (uint32_t)glm::max(rect.Size.x, 1.0f);
    const uint32_t height = (uint32_t)glm::max(rect.Size.y, 1.0f);
    if (width <= 1 || height <= 1) {
        return;
    }

    Mesh* mesh = ResolveMesh();
    if (mesh) {
        AssetManager::Get().LoadAsset(mesh);
    }
    VertexBuffer* vertexBuffer = mesh ? mesh->GetVertexBuffer() : nullptr;

    EnsureTarget(width, height);
    if (!vertexBuffer) {
        return;
    }

    const Array<SubMesh>& subMeshes = mesh->GetSubMeshes();
    m_Pipelines.Resize(subMeshes.Size());
    Array<Pipeline*> pipelines;
    for (int32_t slot = 0; slot < subMeshes.Size(); slot++) {
        m_Pipelines[slot].Dirty = m_Pipelines[slot].Dirty || m_PipelineDirty;
        pipelines.Add(EnsurePipeline(m_Pipelines[slot], ResolveMaterial(slot)));
    }
    m_PipelineDirty = false;
    if (std::all_of(pipelines.begin(), pipelines.end(), [](Pipeline* InPipeline) { return InPipeline == nullptr; })) {
        return;
    }

    m_ShadowMap.Clear();

    AdvanceMotion((float)InContext.DeltaTime);
    UpdateSceneBuffer((float)InContext.DeltaTime, *mesh);

    PreviewPushData push;
    push.WorldTransform = GetMeshRotation() * glm::translate(Mat4(1.0f), -mesh->GetBoundsCenter());

    if (!m_ShaderData) {
        m_ShaderData = new ShaderData();
    }
    m_ShaderData->Set(push);

    for (int32_t slot = 0; slot < subMeshes.Size(); slot++) {
        if (!pipelines[slot]) {
            continue;
        }
        pipelines[slot]->Bind();
        RenderingAPI::GetInstance()->GetRenderQueue().Push(RenderCommandType::SetShaderData,
                                                           CmdSetShaderData{ m_ShaderData.Get() });
        vertexBuffer->Draw(subMeshes[slot].IndexCount, subMeshes[slot].FirstIndex);
    }

    m_Texture->SetView(m_Target->GetDesc().ColorAttachments[0]);
}

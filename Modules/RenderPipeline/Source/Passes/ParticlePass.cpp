#include "ParticlePass.h"
#include "Assets/Material.h"
#include "Assets/Mesh.h"
#include "Assets/ParticleSystemAsset.h"
#include "Assets/Texture2D.h"
#include "GameFramework/CameraNode.h"
#include "GameFramework/ParticleWorld.h"
#include "GameFramework/StaticMeshNode.h"
#include "GameFramework/World.h"
#include "Particles/ParticleSystemInstance.h"
#include "Rendering/Buffer.h"
#include "Rendering/FrameBuffer.h"
#include "Rendering/Pipeline.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/RenderPipeline.h"
#include "Rendering/RenderStage.h"
#include "Rendering/Sampler.h"
#include "Rendering/Shader.h"
#include "Rendering/ShaderData.h"
#include "Rendering/ShaderLibrary.h"
#include "Rendering/ShaderTemplate.h"
#include "Rendering/Texture.h"
#include "Rendering/VertexBuffer.h"

#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>
#include <numeric>

static constexpr uint32_t s_TextureBinding = 16;
static constexpr uint32_t s_SceneDepthBinding = 17;
static constexpr uint32_t s_DefaultTextureSize = 64;
static constexpr uint64_t s_PipelineRetireFrames = 240;

static const char* s_ShaderKeys[ParticlePass::BlendModeCount] = {
    "/Shaders/Particles/ParticleAlpha.glsl",
    "/Shaders/Particles/ParticleAdditive.glsl",
    "/Shaders/Particles/ParticlePremultiplied.glsl",
    "/Shaders/Particles/ParticleMultiply.glsl"
};

static const char* s_MeshShaderKeys[ParticlePass::BlendModeCount] = {
    "/Shaders/Particles/ParticleMeshAlpha.glsl",
    "/Shaders/Particles/ParticleMeshAdditive.glsl",
    "/Shaders/Particles/ParticleMeshPremultiplied.glsl",
    "/Shaders/Particles/ParticleMeshMultiply.glsl"
};

static Vec3 SafeNormalize(const Vec3& InVector, const Vec3& InFallback) {
    const float length = glm::length(InVector);
    return length > 1e-6f ? InVector / length : InFallback;
}

static Quat RotationFromUp(const Vec3& InDirection) {
    const float cosine = glm::dot(VecUtils::Up, InDirection);
    if (cosine < -0.9999f) {
        return glm::angleAxis(glm::pi<float>(), Vec3(1.0f, 0.0f, 0.0f));
    }
    const Vec3 axis = glm::cross(VecUtils::Up, InDirection);
    return glm::normalize(Quat(1.0f + cosine, axis.x, axis.y, axis.z));
}

static Mat3 RotationOf(const Mat3& InBasis) {
    return Mat3(SafeNormalize(InBasis[0], Vec3(1, 0, 0)), SafeNormalize(InBasis[1], Vec3(0, 1, 0)), SafeNormalize(InBasis[2], Vec3(0, 0, 1)));
}

void ParticlePass::Initialize() {
    for (uint32_t mode = 0; mode < BlendModeCount; mode++) {
        m_Shaders[mode] = ShaderLibrary::CreateShader(s_ShaderKeys[mode]);
        m_MeshShaders[mode] = ShaderLibrary::CreateShader(s_MeshShaderKeys[mode]);
    }
    m_Sampler = Sampler::Create({ FilterMode::Linear, FilterMode::Linear,
                                  AddressMode::Clamp, AddressMode::Clamp, AddressMode::Clamp });
    m_DepthSampler = Sampler::Create({ FilterMode::Nearest, FilterMode::Nearest,
                                       AddressMode::Clamp, AddressMode::Clamp, AddressMode::Clamp });
    m_VertexBuffer = VertexBuffer::CreateDynamic();
    m_InstanceBuffer = VertexBuffer::CreateDynamic();
    m_ShaderData = new ShaderData();
    m_ShaderData->Set(MeshShaderData());
    CreateDefaultTextures();
}

void ParticlePass::CreateDefaultTextures() {
    Array<byte> pixels;
    pixels.Resize(s_DefaultTextureSize * s_DefaultTextureSize * 4);
    for (uint32_t y = 0; y < s_DefaultTextureSize; y++) {
        for (uint32_t x = 0; x < s_DefaultTextureSize; x++) {
            const float dx = ((float)x + 0.5f) / (float)s_DefaultTextureSize * 2.0f - 1.0f;
            const float dy = ((float)y + 0.5f) / (float)s_DefaultTextureSize * 2.0f - 1.0f;
            const float falloff = glm::clamp(1.0f - (dx * dx + dy * dy), 0.0f, 1.0f);
            byte* pixel = &pixels[(int32_t)((y * s_DefaultTextureSize + x) * 4)];
            pixel[0] = 255;
            pixel[1] = 255;
            pixel[2] = 255;
            pixel[3] = (byte)std::lround(falloff * falloff * 255.0f);
        }
    }

    TextureDesc desc;
    desc.Width = s_DefaultTextureSize;
    desc.Height = s_DefaultTextureSize;
    m_DefaultTexture = Texture::Create(pixels.Data(), s_DefaultTextureSize, s_DefaultTextureSize, 4, desc);

    byte white[4] = { 255, 255, 255, 255 };
    desc.Width = 1;
    desc.Height = 1;
    m_WhiteTexture = Texture::Create(white, 1, 1, 4, desc);
}

void ParticlePass::Execute(const RenderContext& InContext) {
    ParticleWorld* particles = InContext.World ? InContext.World->GetSubsystem<ParticleWorld>() : nullptr;
    FrameBuffer* target = GetStage()->GetTarget();
    if (!target) {
        return;
    }

    m_Pipelines.Filter([frame = InContext.FrameIndex](const CachedPipeline& InCached) {
        return frame - InCached.LastUsedFrame < s_PipelineRetireFrames;
    });

    if (!particles || !InContext.Camera || !InContext.SceneUniforms || !m_VertexBuffer.Get() || !m_DefaultTexture.Get()) {
        return;
    }

    const CameraNode& camera = *InContext.Camera;
    const Vec3 eye = camera.GetPosition();
    const Vec3 forward = camera.GetForwardVector();

    m_VisibleInstances.Clear();
    m_InstanceDistances.Clear();
    for (const SharedObjectPtr<ParticleSystemInstance>& instance : particles->GetInstances()) {
        if (instance.Get() && instance->GetParticleCount() > 0 && instance->GetSystem()) {
            m_VisibleInstances.Add(instance.Get());
            m_InstanceDistances.Add(glm::dot(Vec3(instance->GetTransform()[3]) - eye, forward));
        }
    }
    if (m_VisibleInstances.IsEmpty()) {
        return;
    }

    Array<int32_t> instanceOrder(m_VisibleInstances.Size());
    std::iota(instanceOrder.begin(), instanceOrder.end(), 0);
    std::stable_sort(instanceOrder.begin(), instanceOrder.end(), [this](int32_t InA, int32_t InB) {
        return m_InstanceDistances[InA] > m_InstanceDistances[InB];
    });

    m_Vertices.Clear();
    m_Indices.Clear();
    m_MeshInstances.Clear();
    m_Batches.Clear();
    for (int32_t index : instanceOrder) {
        const ParticleSystemInstance& instance = *m_VisibleInstances[index];
        const int32_t count = glm::min(instance.GetEmitterCount(), instance.GetSystem()->GetEmitters().Size());
        for (int32_t emitter = 0; emitter < count; emitter++) {
            AppendEmitter(instance, emitter, camera);
        }
    }
    if (m_Batches.IsEmpty()) {
        return;
    }

    RenderStage* geometry = GetStage()->GetPipeline()->FindStage(RenderStageName::DeferredGeometry);
    ImageView* sceneDepth = geometry && geometry->GetTarget() ? geometry->GetTarget()->GetDesc().DepthAttachment.Get() : nullptr;
    RenderCommandQueue& renderQueue = RenderingAPI::GetInstance()->GetRenderQueue();

    if (!m_Indices.IsEmpty()) {
        m_VertexBuffer->Update(m_Vertices.Data(), (uint32_t)(m_Vertices.Size() * sizeof(ParticleVertex)), m_Indices);
    }
    if (!m_MeshInstances.IsEmpty()) {
        m_InstanceBuffer->Update(m_MeshInstances.Data(), (uint32_t)(m_MeshInstances.Size() * sizeof(ParticleMeshInstance)), {});
    }
    int32_t meshShaderData = 0;
    for (const Batch& batch : m_Batches) {
        if (batch.MeshBuffer && batch.MaterialPtr) {
            if (Pipeline* pipeline = m_MeshMaterialPipelines.Resolve(*batch.MaterialPtr, *target, InContext)) {
                pipeline->Bind();
                for (uint32_t i = 0; i < batch.InstanceCount; i++) {
                    if (meshShaderData == m_MeshShaderData.Size()) {
                        m_MeshShaderData.Add(SharedObjectPtr<ShaderData>(new ShaderData()));
                    }
                    const ParticleMeshInstance& instance = m_MeshInstances[(int32_t)(batch.FirstInstance + i)];
                    const Quat rotation(instance.Rotation.w, instance.Rotation.x, instance.Rotation.y, instance.Rotation.z);
                    MeshShaderData data;
                    data.WorldTransform = glm::translate(Mat4(1.0f), instance.Position) * glm::mat4_cast(rotation) * glm::scale(Mat4(1.0f), instance.Scale);
                    ShaderData* shaderData = m_MeshShaderData[meshShaderData++].Get();
                    shaderData->Set(data);
                    renderQueue.Push(RenderCommandType::SetShaderData, CmdSetShaderData{ shaderData });
                    batch.MeshBuffer->Draw();
                }
            }
        } else if (batch.MeshBuffer) {
            if (Pipeline* pipeline = FindPipeline(batch, InContext, sceneDepth)) {
                pipeline->Bind();
                batch.MeshBuffer->DrawInstanced(*m_InstanceBuffer, batch.InstanceCount, batch.FirstInstance);
            }
        } else if (batch.MaterialPtr) {
            if (Pipeline* pipeline = m_MaterialPipelines.Resolve(*batch.MaterialPtr, *target, InContext)) {
                pipeline->Bind();
                renderQueue.Push(RenderCommandType::SetShaderData, CmdSetShaderData{ m_ShaderData });
                m_VertexBuffer->Draw(batch.IndexCount, batch.FirstIndex);
            }
        } else if (Pipeline* pipeline = FindPipeline(batch, InContext, sceneDepth)) {
            pipeline->Bind();
            m_VertexBuffer->Draw(batch.IndexCount, batch.FirstIndex);
        }
    }
}

SharedObjectPtr<ImageView> ParticlePass::GetView(const WeakObjectPtr<Texture2D>& InTexture, Texture* InFallback) const {
    Texture2D* asset = GetLoadedAsset(InTexture);
    Texture* texture = asset ? asset->GetTexture() : nullptr;
    return texture ? texture->GetDefaultView() : InFallback->GetDefaultView();
}

void ParticlePass::AppendEmitter(const ParticleSystemInstance& InInstance, int32_t InEmitter, const CameraNode& InCamera) {
    const ParticleEmitter& settings = InInstance.GetSystem()->GetEmitters()[InEmitter];
    const ParticleEmitterInstance& emitter = InInstance.GetEmitter(InEmitter);
    const ParticleRendererModule& renderer = settings.Renderer;
    if (!settings.Enabled || emitter.GetParticles().IsEmpty()) {
        return;
    }

    Material* material = GetLoadedAsset(renderer.CustomMaterial);
    if (material && (!material->IsReadyToRender() || material->GetShader()->GetRenderState().Path != RenderPath::Forward)) {
        material = nullptr;
    }
    const Vec4 params(glm::max(renderer.SoftParticleDistance, 0.0f), renderer.CameraFadeNear, renderer.CameraFadeFar, renderer.Lit ? 1.0f : 0.0f);
    const Mat4& transform = InInstance.GetTransform();

    uint32_t first = (uint32_t)m_Indices.Size();
    if (renderer.RenderMode != ParticleRenderMode::None) {
        SortParticles(emitter, renderer, transform, InCamera);
    }
    const SharedObjectPtr<ImageView> view = GetView(renderer.Texture, m_DefaultTexture.Get());
    if (renderer.RenderMode == ParticleRenderMode::Mesh) {
        if (Mesh* mesh = GetLoadedAsset(renderer.RenderMesh)) {
            AppendMeshes(emitter, settings, *mesh, transform, params, view, material);
        }
    } else if (renderer.RenderMode != ParticleRenderMode::None) {
        AppendBillboards(emitter, settings, transform, InCamera, params);
        AddBatch(renderer.BlendMode, view, material, first);
    }

    if (settings.Trails.Enabled) {
        first = (uint32_t)m_Indices.Size();
        AppendTrails(emitter, settings, transform, InCamera, params);
        AddBatch(renderer.BlendMode, GetView(settings.Trails.Texture, m_WhiteTexture.Get()), nullptr, first);
    }
}

void ParticlePass::SortParticles(const ParticleEmitterInstance& InEmitter, const ParticleRendererModule& InRenderer, const Mat4& InTransform,
                                 const CameraNode& InCamera) {
    const Array<Particle>& particles = InEmitter.GetParticles();
    const int32_t count = particles.Size();
    const Vec3 eye = InCamera.GetPosition();
    const Vec3 forward = InCamera.GetForwardVector();

    m_WorldPositions.Resize(count);
    m_SortKeys.Resize(count);
    m_Order.Resize(count);
    for (int32_t i = 0; i < count; i++) {
        m_WorldPositions[i] = InEmitter.ToWorldPosition(particles[i].Position, InTransform);
        switch (InRenderer.SortMode) {
            case ParticleSortMode::ByDistance:      m_SortKeys[i] = -glm::dot(m_WorldPositions[i] - eye, forward); break;
            case ParticleSortMode::OldestInFront:   m_SortKeys[i] = particles[i].Age; break;
            case ParticleSortMode::YoungestInFront: m_SortKeys[i] = -particles[i].Age; break;
            case ParticleSortMode::None:            m_SortKeys[i] = 0.0f; break;
        }
    }
    std::iota(m_Order.begin(), m_Order.end(), 0);
    if (InRenderer.SortMode != ParticleSortMode::None) {
        std::sort(m_Order.begin(), m_Order.end(), [this](int32_t InA, int32_t InB) {
            return m_SortKeys[InA] < m_SortKeys[InB];
        });
    }
}

void ParticlePass::AppendBillboards(const ParticleEmitterInstance& InEmitter, const ParticleEmitter& InSettings, const Mat4& InTransform,
                                    const CameraNode& InCamera, const Vec4& InParams) {
    const ParticleRendererModule& renderer = InSettings.Renderer;
    const ParticleTextureSheetModule& sheet = InSettings.TextureSheetAnimation;
    const Array<Particle>& particles = InEmitter.GetParticles();

    const Mat3 basis = Mat3(InTransform);
    const bool local = InEmitter.GetSimulationSpace() == ParticleSimulationSpace::Local;
    const float scale = local ? ParticleEmitterInstance::AverageScale(basis) : 1.0f;
    const Vec2 axes = Vec2(InSettings.Main.StartSizeAxes) * scale;
    const float brightness = glm::max(renderer.Brightness, 0.0f);

    const Vec3 eye = InCamera.GetPosition();
    const Vec3 right = InCamera.GetRightVector();
    const Vec3 up = InCamera.GetUpVector();
    const Vec3 forward = InCamera.GetForwardVector();
    const bool orthographic = InCamera.GetProjectionType() == CameraNode::ProjectionType::Orthographic;

    const int32_t tilesX = sheet.Enabled ? glm::max(sheet.TilesX, 1) : 1;
    const int32_t tilesY = sheet.Enabled ? glm::max(sheet.TilesY, 1) : 1;
    const int32_t tiles = tilesX * tilesY;
    const int32_t frames = sheet.Enabled && sheet.FrameCount > 0 ? glm::min(sheet.FrameCount, tiles) : tiles;
    const Vec2 tileSize(1.0f / (float)tilesX, 1.0f / (float)tilesY);
    const auto tileOrigin = [&](int32_t InFrame) {
        return Vec2((float)(InFrame % tilesX), (float)(InFrame / tilesX)) * tileSize;
    };

    for (int32_t index : m_Order) {
        const Particle& particle = particles[index];
        const Vec3 center = m_WorldPositions[index];
        const Vec2 half = 0.5f * particle.CurrentSize * axes;
        const Color color = Color(Vec3(particle.CurrentColor) * brightness, particle.CurrentColor.a);
        if (half.x <= 0.0f || half.y <= 0.0f || color.a <= 0.0f) {
            continue;
        }

        const Vec3 toEye = orthographic ? -forward : SafeNormalize(eye - center, -forward);
        const float cosine = std::cos(particle.Rotation.z);
        const float sine = std::sin(particle.Rotation.z);
        Vec3 axisX = (right * cosine + up * sine) * half.x;
        Vec3 axisY = (up * cosine - right * sine) * half.y;

        switch (renderer.RenderMode) {
            case ParticleRenderMode::HorizontalBillboard:
                axisX = Vec3(cosine, 0.0f, -sine) * half.x;
                axisY = Vec3(sine, 0.0f, cosine) * half.y;
                break;
            case ParticleRenderMode::VerticalBillboard: {
                const Vec3 side = SafeNormalize(glm::cross(VecUtils::Up, toEye), right);
                axisX = (side * cosine + VecUtils::Up * sine) * half.x;
                axisY = (VecUtils::Up * cosine - side * sine) * half.y;
                break;
            }
            case ParticleRenderMode::StretchedBillboard: {
                const Vec3 velocity = InEmitter.ToWorldDirection(particle.CurrentVelocity, InTransform);
                const float speed = glm::length(velocity);
                const Vec3 side = glm::cross(velocity, toEye);
                if (speed < 1e-5f || glm::length(side) < 1e-6f) {
                    break;
                }
                axisX = glm::normalize(side) * half.x;
                axisY = velocity / speed * (half.y * glm::max(renderer.LengthScale, 0.0f) + 0.5f * speed * glm::max(renderer.SpeedScale, 0.0f));
                break;
            }
            default:
                break;
        }

        Vec2 uvMin(0.0f);
        Vec2 nextMin(0.0f);
        float blend = 0.0f;
        if (frames > 1) {
            const float lifetime = glm::clamp(particle.Age / particle.Lifetime, 0.0f, 1.0f);
            const float start = sheet.RandomStartFrame ? std::floor(particle.Random * (float)frames) : 0.0f;
            const float position = (sheet.FramesPerSecond > 0.0f ? particle.Age * sheet.FramesPerSecond
                                                                  : lifetime * glm::max(sheet.Cycles, 0.0f) * (float)frames) + start;
            const int32_t frame = (int32_t)std::floor(position) % frames;
            uvMin = tileOrigin(frame);
            nextMin = tileOrigin((frame + 1) % frames);
            blend = sheet.FrameBlending ? position - std::floor(position) : 0.0f;
        }

        Vec3 face = SafeNormalize(glm::cross(axisX, axisY), toEye);
        face = glm::dot(face, toEye) < 0.0f ? -face : face;
        const uint32_t base = (uint32_t)m_Vertices.Size();
        const auto corner = [&](float InX, float InY) {
            const Vec2 uv(InX * 0.5f + 0.5f, 0.5f - InY * 0.5f);
            const Vec3 normal = SafeNormalize(face + SafeNormalize(axisX * InX + axisY * InY, face) * 0.5f, face);
            m_Vertices.Add(ParticleVertex{ center + axisX * InX + axisY * InY, color, uvMin + uv * tileSize, normal,
                                           Vec3(nextMin + uv * tileSize, blend), InParams });
        };
        corner(-1.0f, -1.0f);
        corner(-1.0f, 1.0f);
        corner(1.0f, 1.0f);
        corner(1.0f, -1.0f);

        const uint32_t quad[6] = { base, base + 1, base + 2, base, base + 2, base + 3 };
        for (uint32_t vertex : quad) {
            m_Indices.Add(vertex);
        }
    }
}

void ParticlePass::AppendMeshes(const ParticleEmitterInstance& InEmitter, const ParticleEmitter& InSettings, const Mesh& InMesh,
                                const Mat4& InTransform, const Vec4& InParams, const SharedObjectPtr<ImageView>& InView, Material* InMaterial) {
    VertexBuffer* buffer = InMesh.GetVertexBuffer();
    if (!buffer) {
        return;
    }
    const ParticleRendererModule& renderer = InSettings.Renderer;
    const Array<Particle>& particles = InEmitter.GetParticles();
    const uint32_t first = (uint32_t)m_MeshInstances.Size();

    const Mat3 basis = Mat3(InTransform);
    const bool local = InEmitter.GetSimulationSpace() == ParticleSimulationSpace::Local;
    const Quat systemRotation = local ? glm::quat_cast(RotationOf(basis)) : Quat(1.0f, 0.0f, 0.0f, 0.0f);
    const Vec3 axes = InSettings.Main.StartSizeAxes * (local ? ParticleEmitterInstance::AverageScale(basis) : 1.0f);
    const float brightness = glm::max(renderer.Brightness, 0.0f);

    for (int32_t index : m_Order) {
        const Particle& particle = particles[index];
        const Vec3 size = particle.CurrentSize * axes;
        const Color color = Color(Vec3(particle.CurrentColor) * brightness, particle.CurrentColor.a);
        if (particle.CurrentSize <= 0.0f || color.a <= 0.0f) {
            continue;
        }

        Quat rotation = systemRotation * Quat(particle.Rotation);
        if (renderer.AlignToVelocity) {
            const Vec3 velocity = InEmitter.ToWorldDirection(particle.CurrentVelocity, InTransform);
            rotation = RotationFromUp(SafeNormalize(velocity, VecUtils::Up)) * Quat(particle.Rotation);
        }

        rotation = glm::normalize(rotation);
        m_MeshInstances.Add(ParticleMeshInstance{ m_WorldPositions[index], size, Vec4(rotation.x, rotation.y, rotation.z, rotation.w), color, InParams });
    }
    AddMeshBatch(renderer.BlendMode, InView, InMaterial, buffer, first);
}

void ParticlePass::AppendTrails(const ParticleEmitterInstance& InEmitter, const ParticleEmitter& InSettings, const Mat4& InTransform,
                                const CameraNode& InCamera, const Vec4& InParams) {
    const ParticleTrailModule& trails = InSettings.Trails;
    const Array<Particle>& particles = InEmitter.GetParticles();
    const float scale = InEmitter.GetSimulationSpace() == ParticleSimulationSpace::Local ? ParticleEmitterInstance::AverageScale(Mat3(InTransform)) : 1.0f;
    const float brightness = glm::max(InSettings.Renderer.Brightness, 0.0f);

    const auto addPoint = [&](const Particle& InParticle, const Vec3& InPosition, float InTrail) {
        const Color tint = trails.InheritParticleColor ? InParticle.CurrentColor : Color(1.0f);
        const Color color = tint * trails.ColorOverTrail.Evaluate(InTrail);
        const float width = InParticle.CurrentSize * scale * glm::max(trails.Width * trails.WidthOverTrail.Evaluate(InTrail), 0.0f);
        m_TrailPoints.Add({ InEmitter.ToWorldPosition(InPosition, InTransform), width, Color(Vec3(color) * brightness, color.a) });
    };

    m_TrailPoints.Clear();
    if (trails.Mode == ParticleTrailMode::Ribbon) {
        Array<int32_t> order(particles.Size());
        std::iota(order.begin(), order.end(), 0);
        std::sort(order.begin(), order.end(), [&particles](int32_t InA, int32_t InB) { return particles[InA].Age < particles[InB].Age; });
        for (int32_t i = 0; i < order.Size(); i++) {
            addPoint(particles[order[i]], particles[order[i]].Position, order.Size() > 1 ? (float)i / (float)order.Last() : 0.0f);
        }
        AppendStrip(m_TrailPoints, InCamera, InParams);
        return;
    }

    const float lifetime = glm::max(trails.Lifetime, 1e-4f);
    for (const Particle& particle : particles) {
        m_TrailPoints.Clear();
        addPoint(particle, particle.Position, 0.0f);
        for (int32_t i = particle.Trail.Last(); i >= 0; i--) {
            const Vec4& point = particle.Trail[i];
            addPoint(particle, Vec3(point), glm::clamp((particle.Age - point.w) / lifetime, 0.0f, 1.0f));
        }
        AppendStrip(m_TrailPoints, InCamera, InParams);
    }
}

void ParticlePass::AppendStrip(const Array<TrailPoint>& InPoints, const CameraNode& InCamera, const Vec4& InParams) {
    if (InPoints.Size() < 2) {
        return;
    }
    const Vec3 eye = InCamera.GetPosition();
    const Vec3 forward = InCamera.GetForwardVector();
    const bool orthographic = InCamera.GetProjectionType() == CameraNode::ProjectionType::Orthographic;

    const uint32_t base = (uint32_t)m_Vertices.Size();
    for (int32_t i = 0; i < InPoints.Size(); i++) {
        const TrailPoint& point = InPoints[i];
        const Vec3 tangent = InPoints[glm::min(i + 1, InPoints.Last())].Position - InPoints[glm::max(i - 1, 0)].Position;
        const Vec3 toEye = orthographic ? -forward : SafeNormalize(eye - point.Position, -forward);
        const Vec3 side = SafeNormalize(glm::cross(tangent, toEye), InCamera.GetRightVector()) * (0.5f * point.Width);
        const float u = (float)i / (float)InPoints.Last();
        m_Vertices.Add(ParticleVertex{ point.Position - side, point.PointColor, Vec2(u, 1.0f), toEye, Vec3(u, 1.0f, 0.0f), InParams });
        m_Vertices.Add(ParticleVertex{ point.Position + side, point.PointColor, Vec2(u, 0.0f), toEye, Vec3(u, 0.0f, 0.0f), InParams });
    }
    for (uint32_t i = 0; i + 1 < (uint32_t)InPoints.Size(); i++) {
        const uint32_t a = base + i * 2;
        const uint32_t quad[6] = { a, a + 1, a + 3, a, a + 3, a + 2 };
        for (uint32_t vertex : quad) {
            m_Indices.Add(vertex);
        }
    }
}

void ParticlePass::AddMeshBatch(ParticleBlendMode InBlend, const SharedObjectPtr<ImageView>& InView, Material* InMaterial, VertexBuffer* InMesh,
                                uint32_t InFirstInstance) {
    const uint32_t count = (uint32_t)m_MeshInstances.Size() - InFirstInstance;
    if (count == 0) {
        return;
    }
    if (!m_Batches.IsEmpty()) {
        Batch& last = m_Batches.LastItem();
        if (last.MeshBuffer == InMesh && last.Blend == InBlend && last.View.Get() == InView.Get() && last.MaterialPtr == InMaterial
            && last.FirstInstance + last.InstanceCount == InFirstInstance) {
            last.InstanceCount += count;
            return;
        }
    }
    Batch batch;
    batch.Blend = InBlend;
    batch.View = InView;
    batch.MaterialPtr = InMaterial;
    batch.MeshBuffer = InMesh;
    batch.FirstInstance = InFirstInstance;
    batch.InstanceCount = count;
    m_Batches.Add(batch);
}

void ParticlePass::AddBatch(ParticleBlendMode InBlend, const SharedObjectPtr<ImageView>& InView, Material* InMaterial, uint32_t InFirstIndex) {
    const uint32_t count = (uint32_t)m_Indices.Size() - InFirstIndex;
    if (count == 0) {
        return;
    }
    if (!m_Batches.IsEmpty()) {
        Batch& last = m_Batches.LastItem();
        if (!last.MeshBuffer && last.Blend == InBlend && last.View.Get() == InView.Get() && last.MaterialPtr == InMaterial) {
            last.IndexCount += count;
            return;
        }
    }
    Batch batch;
    batch.Blend = InBlend;
    batch.View = InView;
    batch.MaterialPtr = InMaterial;
    batch.FirstIndex = InFirstIndex;
    batch.IndexCount = count;
    m_Batches.Add(batch);
}

Pipeline* ParticlePass::FindPipeline(const Batch& InBatch, const RenderContext& InContext, ImageView* InSceneDepth) {
    FrameBuffer* target = GetStage()->GetTarget();
    ImageView* depth = InSceneDepth ? InSceneDepth : m_DefaultTexture->GetDefaultView().Get();
    const bool instanced = InBatch.MeshBuffer != nullptr;
    const Array<void*> resources = { target, InContext.SceneUniforms, InContext.ShadowMap, depth, InBatch.View.Get() };
    for (CachedPipeline& cached : m_Pipelines) {
        if (cached.Blend == InBatch.Blend && cached.Instanced == instanced && cached.Resources == resources) {
            cached.LastUsedFrame = InContext.FrameIndex;
            return cached.Instance.Get();
        }
    }

    const uint32_t mode = (uint32_t)InBatch.Blend;
    SharedObjectPtr<Shader>* shaders = instanced ? m_MeshShaders : m_Shaders;
    if (mode >= BlendModeCount || !shaders[mode].Get()) {
        return nullptr;
    }

    PipelineDesc desc;
    desc.Target = target;
    desc.Shader = shaders[mode];
    if (instanced) {
        desc.VertexLayout = Vertex::GetLayout();
        desc.InstanceLayout = ParticleMeshInstance::GetLayout();
    } else {
        desc.VertexLayout = ParticleVertex::GetLayout();
    }
    desc.Buffers.Add(InContext.SceneUniforms);
    desc.ImageBindings.Add({ s_TextureBinding, InBatch.View, m_Sampler });
    desc.ImageBindings.Add({ ShaderTemplate::ShadowMapBinding, InContext.ShadowMap, InContext.ShadowSampler });
    desc.ImageBindings.Add({ s_SceneDepthBinding, depth, m_DepthSampler });

    CachedPipeline cached;
    cached.Blend = InBatch.Blend;
    cached.Instanced = instanced;
    cached.Resources = resources;
    cached.Instance = Pipeline::Create(desc);
    cached.LastUsedFrame = InContext.FrameIndex;
    m_Pipelines.Add(cached);
    return m_Pipelines.LastItem().Instance.Get();
}

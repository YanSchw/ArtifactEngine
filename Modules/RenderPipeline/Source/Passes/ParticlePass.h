#pragma once
#include "Rendering/RenderPass.h"
#include "Particles/ParticleModules.h"
#include "MaterialPipelineCache.h"
#include "ParticlePass.gen.h"

class CameraNode;
class FrameBuffer;
class ImageView;
class Pipeline;
class ParticleEmitterInstance;
class ParticleSystemInstance;
class Sampler;
class Shader;
class ShaderData;
class Texture;
class VertexBuffer;

struct ParticleVertex {
    Vec3 Position;
    Color VertexColor;
    Vec2 UV;
    Vec3 Normal;
    /** xy is the next flipbook frame, z how far to blend into it. */
    Vec3 BlendUV;
    /** Soft particle distance, camera fade near and far, lit. */
    Vec4 Params;

    static Array<ShaderDataType> GetLayout() {
        return { ShaderDataType::Float3, ShaderDataType::Float4, ShaderDataType::Float2,
                 ShaderDataType::Float3, ShaderDataType::Float3, ShaderDataType::Float4 };
    }
};

struct ParticleMeshInstance {
    Vec3 Position;
    Vec3 Scale;
    /** Quaternion as xyzw. */
    Vec4 Rotation;
    Color InstanceColor;
    Vec4 Params;

    static Array<ShaderDataType> GetLayout() {
        return { ShaderDataType::Float3, ShaderDataType::Float3, ShaderDataType::Float4, ShaderDataType::Float4, ShaderDataType::Float4 };
    }
};

class ParticlePass : public RenderPass {
public:
    ARTIFACT_CLASS();

    static constexpr uint32_t BlendModeCount = 4;

    virtual RenderStageName GetStageName() const override { return RenderStageName::Forward; }
    virtual int32_t GetOrderPreferenceIndex() const override { return 100; }

protected:
    virtual void Initialize() override;
    virtual void Execute(const RenderContext& InContext) override;

private:
    struct Batch {
        ParticleBlendMode Blend = ParticleBlendMode::Alpha;
        SharedObjectPtr<ImageView> View;
        Material* MaterialPtr = nullptr;
        uint32_t FirstIndex = 0;
        uint32_t IndexCount = 0;
        VertexBuffer* MeshBuffer = nullptr;
        uint32_t FirstInstance = 0;
        uint32_t InstanceCount = 0;
    };

    struct CachedPipeline {
        ParticleBlendMode Blend = ParticleBlendMode::Alpha;
        bool Instanced = false;
        Array<void*> Resources;
        SharedObjectPtr<Pipeline> Instance;
        uint64_t LastUsedFrame = 0;
    };

    struct TrailPoint {
        Vec3 Position;
        float Width = 0.0f;
        Color PointColor;
    };

    void AppendEmitter(const ParticleSystemInstance& InInstance, int32_t InEmitter, const CameraNode& InCamera);
    void SortParticles(const ParticleEmitterInstance& InEmitter, const ParticleRendererModule& InRenderer, const Mat4& InTransform, const CameraNode& InCamera);
    void AppendBillboards(const ParticleEmitterInstance& InEmitter, const ParticleEmitter& InSettings, const Mat4& InTransform,
                          const CameraNode& InCamera, const Vec4& InParams);
    void AppendMeshes(const ParticleEmitterInstance& InEmitter, const ParticleEmitter& InSettings, const Mesh& InMesh,
                      const Mat4& InTransform, const Vec4& InParams, const SharedObjectPtr<ImageView>& InView, Material* InMaterial);
    void AppendTrails(const ParticleEmitterInstance& InEmitter, const ParticleEmitter& InSettings, const Mat4& InTransform,
                      const CameraNode& InCamera, const Vec4& InParams);
    void AppendStrip(const Array<TrailPoint>& InPoints, const CameraNode& InCamera, const Vec4& InParams);
    void AddBatch(ParticleBlendMode InBlend, const SharedObjectPtr<ImageView>& InView, Material* InMaterial, uint32_t InFirstIndex);
    void AddMeshBatch(ParticleBlendMode InBlend, const SharedObjectPtr<ImageView>& InView, Material* InMaterial, VertexBuffer* InMesh, uint32_t InFirstInstance);
    Pipeline* FindPipeline(const Batch& InBatch, const RenderContext& InContext, ImageView* InSceneDepth);
    SharedObjectPtr<ImageView> GetView(const WeakObjectPtr<Texture2D>& InTexture, Texture* InFallback) const;
    void CreateDefaultTextures();

    SharedObjectPtr<Shader> m_Shaders[BlendModeCount];
    SharedObjectPtr<Shader> m_MeshShaders[BlendModeCount];
    SharedObjectPtr<Sampler> m_Sampler;
    SharedObjectPtr<Sampler> m_DepthSampler;
    SharedObjectPtr<Texture> m_DefaultTexture;
    SharedObjectPtr<Texture> m_WhiteTexture;
    SharedObjectPtr<VertexBuffer> m_VertexBuffer;
    SharedObjectPtr<VertexBuffer> m_InstanceBuffer;
    SharedObjectPtr<ShaderData> m_ShaderData;
    MaterialPipelineCache m_MaterialPipelines{ ParticleVertex::GetLayout() };
    MaterialPipelineCache m_MeshMaterialPipelines;
    Array<SharedObjectPtr<ShaderData>> m_MeshShaderData;

    Array<ParticleVertex> m_Vertices;
    Array<uint32_t> m_Indices;
    Array<ParticleMeshInstance> m_MeshInstances;
    Array<Batch> m_Batches;
    Array<CachedPipeline> m_Pipelines;
    Array<int32_t> m_Order;
    Array<float> m_SortKeys;
    Array<Vec3> m_WorldPositions;
    Array<TrailPoint> m_TrailPoints;
    Array<const ParticleSystemInstance*> m_VisibleInstances;
    Array<float> m_InstanceDistances;
};

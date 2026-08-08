#pragma once
#include "Node3D.h"
#include "Object/Pointer.h"
#include "Rendering/SceneUniforms.h"
#include "DirectionalLightNode.gen.h"

class CameraNode;
class FrameBuffer;
class Image;
class ImageView;
class Pipeline;
class Sampler;
class Shader;
class UniformBuffer;
class World;

/** A 1x1 stand-in a renderer binds while no light has rendered a shadow map */
struct ShadowMapPlaceholder {
    void Create();
    void Clear() const;

    ImageView* GetView() const;
    Sampler* GetSampler() const;

private:
    SharedObjectPtr<Image> m_Image;
    SharedObjectPtr<ImageView> m_View;
    SharedObjectPtr<FrameBuffer> m_Target;
    SharedObjectPtr<Sampler> m_Sampler;
};

/** A sun: an infinitely distant light travelling along this node's forward vector. */
class DirectionalLightNode : public Node3D {
public:
    ARTIFACT_CLASS();

    static constexpr int32_t CascadeCount = SceneUniformData::ShadowCascadeCount;
    static constexpr const char* ShadowShaderKey = "/Shaders/ShadowDepth.glsl";

    DirectionalLightNode();
    ~DirectionalLightNode();

    Vec3 GetDirection() const { return GetForwardVector(); }

    Vec3 GetColor() const { return m_Color; }
    void SetColor(const Vec3& InColor) { m_Color = InColor; }

    float GetIntensity() const { return m_Intensity; }
    void SetIntensity(float InIntensity) { m_Intensity = InIntensity; }

    Vec3 GetAmbientColor() const { return m_AmbientColor; }
    void SetAmbientColor(const Vec3& InColor) { m_AmbientColor = InColor; }

    float GetAmbientIntensity() const { return m_AmbientIntensity; }
    void SetAmbientIntensity(float InIntensity) { m_AmbientIntensity = InIntensity; }

    bool GetCastShadows() const { return m_CastShadows; }
    void SetCastShadows(bool InCastShadows);

    /** Fits the cascades to InCamera's view and records one depth-only pass per cascade, drawing
     *  every shadow-casting StaticMeshNode of InWorld. Returns false while shadows are off or the
     *  targets could not be built, in which case nothing was recorded. */
    bool RenderShadowMaps(World& InWorld, const CameraNode& InCamera);

    ImageView* GetShadowMapView() const;
    Sampler* GetShadowSampler() const;

    const Mat4& GetCascadeMatrix(int32_t InCascade) const;
    /** World-space size of one shadow texel, per cascade. */
    Vec4 GetCascadeTexelSizes() const { return m_CascadeTexelSizes; }
    /** Matches SceneUniformData::ShadowParams: x depth bias, y normal bias in texels, z cascade
     *  count, w the uv distance between neighbouring filter taps. */
    Vec4 GetShadowParams() const;

private:
    uint32_t GetClampedResolution() const;
    bool EnsureShadowMaps();
    void ReleaseShadowMaps();
    void UpdateCascades(const CameraNode& InCamera);
    void RecordCascadePass(World& InWorld, int32_t InCascade);

    PROPERTY()
    Vec3 m_Color = Vec3(1.0f, 0.97f, 0.91f);

    PROPERTY()
    float m_Intensity = 1.2f;

    PROPERTY()
    Vec3 m_AmbientColor = Vec3(0.42f, 0.5f, 0.62f);

    PROPERTY()
    float m_AmbientIntensity = 0.35f;

    PROPERTY()
    bool m_CastShadows = true;

    PROPERTY()
    uint32_t m_ShadowMapResolution = 2048;

    /** How far from the camera the last cascade reaches. */
    PROPERTY()
    float m_ShadowDistance = 100.0f;

    /** 0 spaces the cascades evenly, 1 spaces them logarithmically. */
    PROPERTY()
    float m_CascadeDistribution = 0.9f;

    /** Spacing between the filter taps, in shadow texels. Raise it for softer edges. */
    PROPERTY()
    float m_Softness = 1.5f;

    PROPERTY()
    float m_DepthBias = 0.0016f;

    /** Offset along the surface normal before the lookup, in shadow texels. */
    PROPERTY()
    float m_NormalBias = 2.0f;

    /** How far behind a cascade its light frustum starts, so casters outside the view still
     *  reach into it. */
    PROPERTY()
    float m_CasterDistance = 100.0f;

    SharedObjectPtr<Image> m_ShadowImage;
    SharedObjectPtr<ImageView> m_ShadowArrayView;
    SharedObjectPtr<Sampler> m_ShadowSampler;
    SharedObjectPtr<Shader> m_ShadowShader;
    Array<SharedObjectPtr<ImageView>> m_CascadeViews;
    Array<SharedObjectPtr<FrameBuffer>> m_CascadeTargets;
    Array<SharedObjectPtr<UniformBuffer>> m_CascadeBuffers;
    Array<SharedObjectPtr<Pipeline>> m_CascadePipelines;
    Array<Mat4> m_CascadeMatrices;
    Vec4 m_CascadeTexelSizes = Vec4(0.0f);
    uint32_t m_ShadowMapsResolution = 0;
};

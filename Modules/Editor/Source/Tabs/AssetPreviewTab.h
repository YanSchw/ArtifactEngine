#pragma once
#include "MinorTab.h"
#include "Object/Pointer.h"
#include "GameFramework/DirectionalLightNode.h"
#include "AssetPreviewTab.gen.h"

class Material;
class Mesh;
class FrameBuffer;
class Pipeline;
class UniformBuffer;
class RenderTargetTexture;
class UIImage;
class Sampler;
class VectorImage;
class UIMenuModel;

enum class PreviewPanMode : uint8_t {
    Rotate,
    PanLeftRight,
    None
};

/** Renders one mesh with one material, framed by the mesh's bounds. Shared by every editor that
 *  shows what an asset looks like: shader graphs, materials and meshes. */
class AssetPreviewTab : public MinorTab {
public:
    ARTIFACT_CLASS();

    AssetPreviewTab();

    virtual String GetTabTitle() const override { return "Preview"; }
    virtual VectorImage* GetTabIcon() const override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;
    virtual void OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) override;
    virtual bool OnSecondaryClick(const Vec2& InCursorPos) override;

    /** Leave unset to preview the mesh's own material. */
    void SetMaterial(Material* InMaterial);
    /** Leave unset to preview on the default cube. */
    void SetMesh(Mesh* InMesh);

    void InvalidatePipeline();

private:
    Material* ResolveMaterial() const;
    Mesh* ResolveMesh() const;

    void EnsureTarget(uint32_t InWidth, uint32_t InHeight);
    void EnsurePipeline();
    void UpdateSceneBuffer(float InDeltaTime, const Mesh& InMesh);

    void BuildMeshMenu(UIMenuModel& OutMenu);
    void BuildPanMenu(UIMenuModel& OutMenu);
    void AdvanceMotion(float InDeltaTime);
    Mat4 GetMeshRotation() const;

    WeakObjectPtr<Material> m_Material;
    WeakObjectPtr<Mesh> m_Mesh;

    SharedObjectPtr<FrameBuffer> m_Target;
    SharedObjectPtr<Pipeline> m_Pipeline;
    SharedObjectPtr<UniformBuffer> m_SceneBuffer;
    SharedObjectPtr<RenderTargetTexture> m_Texture;
    SharedObjectPtr<Sampler> m_Sampler;
    SharedObjectPtr<class ShaderData> m_ShaderData;
    ShadowMapPlaceholder m_ShadowMap;
    Array<void*> m_PipelineResources;
    UIImage* m_Image = nullptr;

    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    float m_Time = 0.0f;
    bool m_PipelineDirty = true;

    PreviewPanMode m_PanMode = PreviewPanMode::Rotate;
    float m_Yaw = 0.0f;
    float m_Pitch = 0.0f;
    float m_SwayPhase = 0.0f;
};

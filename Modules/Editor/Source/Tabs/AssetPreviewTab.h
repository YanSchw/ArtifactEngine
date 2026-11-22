#pragma once
#include "MinorTab.h"
#include "Object/Pointer.h"
#include "AssetPreviewTab.gen.h"

class Material;
class Mesh;
class World;
class CameraNode;
class StaticMeshNode;
class RenderPipeline;
class RenderTargetTexture;
class UIImage;
class VectorImage;
class UIMenuModel;

enum class PreviewPanMode : uint8_t {
    Rotate,
    PanLeftRight,
    None
};

/** Renders one mesh, with one material or the mesh's own per slot, framed by the mesh's bounds.
 *  Shared by every editor that shows what an asset looks like: shader graphs, materials and meshes. */
class AssetPreviewTab : public MinorTab {
public:
    ARTIFACT_CLASS();

    AssetPreviewTab();

    virtual String GetTabTitle() const override { return "Preview"; }
    virtual VectorImage* GetTabIcon() const override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;
    virtual void OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) override;
    virtual bool OnScroll(const Vec2& InDelta) override;
    virtual bool OnSecondaryClick(const Vec2& InCursorPos) override;

    /** Leave unset to preview the mesh's own material. */
    void SetMaterial(Material* InMaterial);
    /** Leave unset to preview on the default cube. */
    void SetMesh(Mesh* InMesh);

private:
    Mesh* ResolveMesh() const;
    void UpdateScene(Mesh& InMesh);

    void BuildMeshMenu(UIMenuModel& OutMenu);
    void BuildPanMenu(UIMenuModel& OutMenu);
    void AdvanceMotion(float InDeltaTime);
    Mat4 GetMeshRotation() const;

    WeakObjectPtr<Material> m_Material;
    WeakObjectPtr<Mesh> m_Mesh;

    SharedObjectPtr<World> m_PreviewWorld;
    SharedObjectPtr<RenderPipeline> m_Pipeline;
    SharedObjectPtr<RenderTargetTexture> m_Texture;
    CameraNode* m_Camera = nullptr;
    StaticMeshNode* m_MeshNode = nullptr;
    UIImage* m_Image = nullptr;

    PreviewPanMode m_PanMode = PreviewPanMode::Rotate;
    float m_Yaw = 0.0f;
    float m_Pitch = 0.0f;
    float m_SwayPhase = 0.0f;
    float m_Zoom = 1.0f;
};

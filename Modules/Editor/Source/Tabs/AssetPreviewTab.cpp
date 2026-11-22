#include "AssetPreviewTab.h"
#include "PreviewEnvironment.h"

#include "Assets/AssetManager.h"
#include "Assets/Material.h"
#include "Assets/Mesh.h"
#include "Rendering/RenderPipeline.h"
#include "Rendering/RenderTargetTexture.h"
#include "GameFramework/CameraNode.h"
#include "GameFramework/StaticMeshNode.h"
#include "GameFramework/UIImage.h"
#include "GameFramework/World.h"
#include "UI/EditorIcons.h"
#include "UI/EditorStyle.h"
#include "UI/UIContextMenu.h"

static const UUID s_DefaultMesh = UUID::FromString("c6308770-3a5b-4b2b-9cec-14ba803ff817");
static constexpr float s_FieldOfView = 45.0f;

static const char* const s_PrimitiveMeshes[] = { "Sphere", "Cube", "Quad" };
static constexpr float s_SwayAmplitude = glm::radians(45.0f);
static constexpr float s_SwaySpeed = 0.9f;
static constexpr float s_SpinSpeed = 0.6f;
static constexpr float s_DragSpeed = 0.008f;
static constexpr float s_PitchLimit = 1.4f;
static constexpr float s_ZoomStep = 0.9f;
static constexpr float s_MinZoom = 0.15f;
static constexpr float s_MaxZoom = 3.0f;

static Mesh* FindMeshNamed(const String& InName) {
    for (Asset* asset : AssetManager::Get().GetAssetsOfClass(Mesh::StaticClass())) {
        Mesh* mesh = Cast<Mesh>(asset);
        if (mesh && mesh->GetDisplayName() == InName) {
            return mesh;
        }
    }
    return nullptr;
}

AssetPreviewTab::AssetPreviewTab() {
    Interactable = true;
    m_Texture = Object::Create<RenderTargetTexture>();

    m_Image = Add<UIImage>();
    m_Image->Fill();
    m_Image->Image = m_Texture;

    m_Pipeline = RenderPipeline::Create();
    m_PreviewWorld = new World();
    m_PreviewWorld->Spawn<PreviewEnvironment>();
    m_MeshNode = m_PreviewWorld->Spawn<StaticMeshNode>();
    m_Camera = m_PreviewWorld->Spawn<CameraNode>();
}

VectorImage* AssetPreviewTab::GetTabIcon() const {
    return EditorIcons::Viewport();
}

void AssetPreviewTab::SetMaterial(Material* InMaterial) {
    m_Material = InMaterial;
    AssetManager::Get().LoadAsset(InMaterial);
}

void AssetPreviewTab::SetMesh(Mesh* InMesh) {
    m_Mesh = InMesh;
    AssetManager::Get().LoadAsset(InMesh);
}

Mesh* AssetPreviewTab::ResolveMesh() const {
    Mesh* mesh = m_Mesh.Get();
    return mesh ? mesh : AssetManager::Get().GetAsset<Mesh>(s_DefaultMesh);
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

bool AssetPreviewTab::OnScroll(const Vec2& InDelta) {
    if (!IsHovered()) {
        return false;
    }
    m_Zoom = glm::clamp(m_Zoom * glm::pow(s_ZoomStep, InDelta.y), s_MinZoom, s_MaxZoom);
    return true;
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

void AssetPreviewTab::UpdateScene(Mesh& InMesh) {
    m_MeshNode->SetMesh(&InMesh);
    for (int32_t slot = 0; slot < InMesh.GetSubMeshes().Size(); slot++) {
        m_MeshNode->SetMaterialOverride(slot, m_Material.Get());
    }
    m_MeshNode->SetTransformMatrix(GetMeshRotation() * glm::translate(Mat4(1.0f), -InMesh.GetBoundsCenter()));

    const float radius = InMesh.GetBoundsRadius();
    const Vec3 eye = Vec3(0.0f, radius * 0.9f, -radius * 3.2f) * m_Zoom;
    m_Camera->SetPerspective(s_FieldOfView, radius * 0.01f, radius * 50.0f);
    m_Camera->SetPosition(eye);
    m_Camera->SetLookDirection(-eye);
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
    if (!mesh) {
        return;
    }
    AssetManager::Get().LoadAsset(mesh);

    AdvanceMotion((float)InContext.DeltaTime);
    UpdateScene(*mesh);

    RenderParams params;
    params.Width = width;
    params.Height = height;
    params.m_World = m_PreviewWorld.Get();
    params.CameraOverride = m_Camera;
    m_Pipeline->Render(InContext.DeltaTime, params);

    m_Texture->SetView(m_Pipeline->GetFinalImageView());
}

#include "ThumbnailRenderer.h"
#include "Assets/AssetManager.h"
#include "Assets/Asset.h"
#include "Assets/Mesh.h"
#include "Assets/Material.h"
#include "Assets/Texture2D.h"
#include "Core/EngineConfig.h"
#include "Rendering/RenderPipeline.h"
#include "Rendering/RenderTargetTexture.h"
#include "GameFramework/World.h"
#include "GameFramework/StaticMeshNode.h"
#include "GameFramework/CameraNode.h"
#include "GameFramework/DirectionalLightNode.h"
#include <cmath>

static constexpr uint32_t s_ThumbnailSize = 128;
static constexpr int32_t s_MaxCache = 128;
static constexpr int32_t s_PerFrameBudget = 2;
static constexpr int32_t s_MaxAttempts = 90;
static constexpr float s_FieldOfView = 35.0f;
static constexpr float s_BoundsPadding = 1.08f;

static constexpr const char* s_SphereMeshId = "f70b2731-f584-44ec-9ccb-d6c9eb7f01b6";
static const Vec3 s_ViewDirection = glm::normalize(Vec3(0.5f, -0.42f, 0.95f));
static const Vec3 s_LightDirection = glm::normalize(Vec3(0.45f, -0.8f, 0.4f));

/** How far back along InForward the camera has to sit for the whole box to fall inside the frame. */
static float FitDistance(const Vec3& InExtents, const Vec3& InForward) {
    const Vec3 right = glm::normalize(glm::cross(VecUtils::Up, InForward));
    const Vec3 up = glm::cross(InForward, right);
    const Vec3 extents = InExtents * s_BoundsPadding;
    const float tanHalfFov = std::tan(glm::radians(s_FieldOfView) * 0.5f);

    float distance = 0.0f;
    for (int32_t corner = 0; corner < 8; corner++) {
        const Vec3 point((corner & 1) ? extents.x : -extents.x,
                         (corner & 2) ? extents.y : -extents.y,
                         (corner & 4) ? extents.z : -extents.z);
        const float offset = glm::max(glm::abs(glm::dot(point, right)), glm::abs(glm::dot(point, up)));
        distance = glm::max(distance, offset / tanHalfFov - glm::dot(point, InForward));
    }
    return glm::max(distance, 0.001f);
}

Texture* ThumbnailRenderer::GetThumbnail(Asset* InAsset) {
    if (!InAsset) {
        return nullptr;
    }

    if (Texture2D* texture = InAsset->As<Texture2D>()) {
        return texture->IsLoaded() ? texture->GetTexture() : nullptr;
    }

    if (!InAsset->IsA(Mesh::StaticClass()) && !InAsset->IsA(Material::StaticClass())) {
        return nullptr;
    }
    const UUID id = InAsset->GetId();
    if (!m_Cache.ContainsKey(id)) {
        if (m_Cache.Size() >= s_MaxCache) {
            return nullptr;
        }
        Entry& entry = m_Cache[id];
        entry.Texture = Object::Create<RenderTargetTexture>();
        entry.Rendered = false;
        m_Pending.Add(id);
    }
    return m_Cache[id].Texture.Get();
}

void ThumbnailRenderer::Tick() {
    int32_t budget = s_PerFrameBudget;
    while (!m_Pending.IsEmpty() && budget > 0) {
        const UUID id = m_Pending[0];
        m_Pending.RemoveAt(0);
        if (!m_Cache.ContainsKey(id)) {
            continue;
        }
        Entry& entry = m_Cache[id];
        if (entry.Rendered) {
            continue;
        }
        RenderEntry(id, entry);
        budget--;
    }
}

void ThumbnailRenderer::RenderEntry(const UUID& InId, Entry& InEntry) {
    Asset* asset = AssetManager::Get().GetAsset(InId);
    Material* material = Cast<Material>(asset);
    // A material has no shape of its own, so it is shown on the same sphere every time.
    Mesh* mesh = material ? AssetManager::Get().GetAsset<Mesh>(UUID::FromString(s_SphereMeshId))
                          : Cast<Mesh>(asset);
    if (!mesh) {
        InEntry.Rendered = true;
        return;
    }

    if (!InEntry.Pipeline.Get()) {
        InEntry.Pipeline = Object::Create(EngineConfig::RenderPipelineClass())->As<RenderPipeline>();

        World* scene = new World();
        StaticMeshNode* meshNode = scene->Spawn<StaticMeshNode>();
        meshNode->SetMesh(mesh);
        meshNode->SetMaterialOverride(material);

        const float distance = FitDistance(mesh->GetBoundsExtents(), s_ViewDirection);

        CameraNode* camera = scene->Spawn<CameraNode>();
        camera->SetPerspective(s_FieldOfView, distance * 0.01f, distance + mesh->GetBoundsRadius() * 4.0f);
        camera->SetPosition(mesh->GetBoundsCenter() - s_ViewDirection * distance);
        camera->SetLookDirection(s_ViewDirection);
        scene->SetMainCamera(camera);

        DirectionalLightNode* light = scene->Spawn<DirectionalLightNode>();
        light->SetCastShadows(false);
        light->SetLookDirection(s_LightDirection);

        InEntry.Scene = scene;
    }

    // Assets stream in behind the scenes and nothing is drawn until they are all there, so an
    // unready material waits its turn again instead of baking an empty thumbnail.
    Material* drawnWith = material ? material : mesh->GetMaterial();
    if ((!drawnWith || !drawnWith->IsReadyToRender()) && ++InEntry.Attempts < s_MaxAttempts) {
        m_Pending.Add(InId);
        return;
    }

    RenderParams params;
    params.Width = s_ThumbnailSize;
    params.Height = s_ThumbnailSize;
    params.m_World = InEntry.Scene.Get();
    InEntry.Pipeline->Render(0.016, params);
    InEntry.Texture->SetView(InEntry.Pipeline->GetFinalImageView());
    InEntry.Rendered = true;
}

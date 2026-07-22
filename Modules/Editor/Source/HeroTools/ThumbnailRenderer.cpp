#include "ThumbnailRenderer.h"
#include "Assets/AssetManager.h"
#include "Assets/Asset.h"
#include "Assets/Mesh.h"
#include "Core/EngineConfig.h"
#include "Rendering/RenderPipeline.h"
#include "Rendering/RenderTargetTexture.h"
#include "GameFramework/World.h"
#include "GameFramework/StaticMeshNode.h"
#include "GameFramework/CameraNode.h"

static constexpr uint32_t s_ThumbnailSize = 128;
static constexpr int32_t s_MaxCache = 128;
static constexpr int32_t s_PerFrameBudget = 2;

Texture* ThumbnailRenderer::GetThumbnail(Asset* InAsset) {
    Mesh* mesh = InAsset ? InAsset->As<Mesh>() : nullptr;
    if (!mesh) {
        return nullptr;
    }
    const UUID id = mesh->GetId();
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
    Mesh* mesh = AssetManager::Get().GetAsset<Mesh>(InId);
    if (!mesh) {
        InEntry.Rendered = true;
        return;
    }

    if (!InEntry.Pipeline.Get()) {
        InEntry.Pipeline = Object::Create(EngineConfig::RenderPipelineClass())->As<RenderPipeline>();

        World* scene = new World();
        StaticMeshNode* meshNode = scene->Spawn<StaticMeshNode>();
        meshNode->SetMesh(mesh);

        CameraNode* camera = scene->Spawn<CameraNode>();
        camera->SetPerspectiveVerticalFOV(40.0f);
        const Vec3 direction = glm::normalize(Vec3(0.9f, 0.75f, 1.5f));
        camera->SetPosition(direction * 3.4f);
        camera->SetLookDirection(-direction);
        scene->SetMainCamera(camera);

        InEntry.Scene = scene;
    }

    RenderParams params;
    params.Width = s_ThumbnailSize;
    params.Height = s_ThumbnailSize;
    params.m_World = InEntry.Scene.Get();
    InEntry.Pipeline->Render(0.016, params);
    InEntry.Texture->SetView(InEntry.Pipeline->GetFinalImageView());
    InEntry.Rendered = true;
}

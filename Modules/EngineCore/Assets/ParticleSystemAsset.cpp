#include "ParticleSystemAsset.h"
#include "AssetManager.h"
#include "Material.h"
#include "Mesh.h"
#include "Texture2D.h"

ParticleSystemAsset* ParticleSystemAsset::CreateEmpty(const String& InDirectory, const String& InName) {
    ParticleSystemAsset* asset = Cast<ParticleSystemAsset>(AssetManager::Get().CreateAsset(StaticClass(), InDirectory, InName));
    if (!asset) {
        return nullptr;
    }

    ParticleEmitter emitter;
    emitter.Main.StartLifetime = ParticleFloatRange(2.0f, 3.0f);
    emitter.Main.StartSpeed = ParticleFloatRange(1.5f, 2.5f);
    emitter.Main.StartSize = ParticleFloatRange(0.3f, 0.5f);
    emitter.Shape.Radius = 0.25f;
    emitter.ColorOverLifetime.Enabled = true;
    asset->m_Emitters.Add(emitter);

    AssetManager::Get().SaveAsset(asset);
    return asset;
}

bool ParticleSystemAsset::IsSubEmitter(int32_t InEmitter) const {
    for (const ParticleEmitter& emitter : m_Emitters) {
        for (const ParticleSubEmitter& sub : emitter.SubEmitters) {
            if (sub.Enabled && sub.EmitterIndex == InEmitter) {
                return true;
            }
        }
    }
    return false;
}

void ParticleSystemAsset::Load() {
    m_Loaded = true;
    AcquireReferencedAssets();
}

void ParticleSystemAsset::Unload() {
    m_Handles.Clear();
    m_Loaded = false;
}

void ParticleSystemAsset::AcquireReferencedAssets() {
    Array<AssetStreamHandle> handles;
    const auto acquire = [&handles](Asset* InAsset) {
        if (InAsset) {
            handles.Add(InAsset->GetStreamHandle());
        }
    };
    for (const ParticleEmitter& emitter : m_Emitters) {
        acquire(emitter.Renderer.Texture.Get());
        acquire(emitter.Renderer.CustomMaterial.Get());
        acquire(emitter.Renderer.RenderMesh.Get());
        acquire(emitter.Trails.Texture.Get());
    }
    m_Handles = std::move(handles);
}

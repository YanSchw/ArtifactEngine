#pragma once
#include "Asset.h"
#include "Particles/ParticleModules.h"
#include "ParticleSystemAsset.gen.h"

class ParticleSystemAsset : public Asset {
public:
    ARTIFACT_CLASS();

    static ParticleSystemAsset* CreateEmpty(const String& InDirectory, const String& InName);

    virtual bool IsLoaded() const override { return m_Loaded; }

    void AcquireReferencedAssets();

    const Array<ParticleEmitter>& GetEmitters() const { return m_Emitters; }
    Array<ParticleEmitter>& GetEmitters() { return m_Emitters; }

    bool IsSubEmitter(int32_t InEmitter) const;

protected:
    virtual void Load() override;
    virtual void Unload() override;

private:
    PROPERTY()
    Array<ParticleEmitter> m_Emitters;

    Array<AssetStreamHandle> m_Handles;
    bool m_Loaded = false;
};

template<typename T>
T* GetLoadedAsset(const WeakObjectPtr<T>& InAsset) {
    T* asset = InAsset.Get();
    return asset && asset->IsLoaded() ? asset : nullptr;
}

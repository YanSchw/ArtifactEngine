#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "Common/UUID.h"
#include "Common/Map.h"
#include "AssetManager.gen.h"

struct AssetStreamHandle;
class Asset;

class AssetManager : public Object {
public:
    ARTIFACT_CLASS();
    virtual ~AssetManager();

    static AssetManager& Get();

    void Initialize(bool InLoadAssets = true);
    void Shutdown();
    void HotLoadAssets();

    void LoadAsset(Asset* InAsset);
    void UnloadAsset(Asset* InAsset);
    Asset* GetAsset(const UUID& InId);

    template<typename T>
    T* GetAsset(const UUID& InId) {
        Object* asset = (Object*)GetAsset(InId);
        return asset ? asset->As<T>() : nullptr;
    }

private:
    static void AssetStreamingThreadFunc();
    Array<Asset*> GetAllAssets() const;

private:
    Map<UUID, SharedObjectPtr<Asset>> m_Assets;

    friend class AssetCookerEngine;
};
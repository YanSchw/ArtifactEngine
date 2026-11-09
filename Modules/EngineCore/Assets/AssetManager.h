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
    static bool IsAvailable();

    void Initialize(bool InLoadAssets = true);
    void Shutdown();
    void HotLoadAssets();

    void LoadAsset(Asset* InAsset);
    void UnloadAsset(Asset* InAsset);
    Asset* GetAsset(const UUID& InId);
    Array<Asset*> GetAssetsOfClass(const Class& InClass) const;

    template<typename T>
    T* GetAsset(const UUID& InId) {
        Object* asset = (Object*)GetAsset(InId);
        return asset ? asset->As<T>() : nullptr;
    }

    String GetAssetPath(const UUID& InId);
    Asset* GetAssetAtPath(const String& InPath) const;
    Asset* CreateAsset(const Class& InClass, const String& InDirectory, const String& InName);
    /** Registers an asset made outside the manager under a fresh id. */
    Asset* AddAsset(Asset* InAsset, const String& InDirectory, const String& InName);
    /** Sub-assets have no file of their own: they are written into their owner's file and their
     *  path is a virtual one inside it, which is what keeps their names unique. */
    Asset* AddSubAsset(Asset* InAsset, Asset* InOwner, const UUID& InId, const String& InName);
    Array<Asset*> GetSubAssets(const Asset* InOwner) const;
    /** Saving a sub-asset saves its owner. */
    bool SaveAsset(Asset* InAsset);

    bool MoveAsset(const UUID& InId, const String& InNewPath);
    bool DeleteAsset(Asset* InAsset);
    void RebindAssetPaths(const String& InOldDirectory, const String& InNewDirectory);
    void UnregisterAssetsUnder(const String& InDirectory);

private:
    static void AssetStreamingThreadFunc();
    Array<Asset*> GetAllAssets() const;

private:
    Map<UUID, SharedObjectPtr<Asset>> m_Assets;
    Map<UUID, String> m_AssetPaths;

    friend class AssetCookerEngine;
};

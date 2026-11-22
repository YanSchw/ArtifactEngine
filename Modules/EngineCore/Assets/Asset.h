#pragma once
#include "Common/UUID.h"
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "Object/Enum.h"
#include "Asset.gen.h"

ARTIFACT_ENUM();
enum class AssetFlags : uint8_t {
    None = 0,
    Missing = 1 << 0
};

ARTIFACT_ENUM();
enum class AssetStreamType : uint8_t {
    Referenced = 0,
    Manual = 1 << 0,
    AlwaysLoaded = 1 << 1
};

class Asset;

struct AssetStreamHandle {
    ARTIFACT_STRUCT();

    AssetStreamHandle() = default;
    explicit AssetStreamHandle(Asset* InAsset);

    AssetStreamHandle(const AssetStreamHandle& Other);
    AssetStreamHandle(AssetStreamHandle&& Other) noexcept;

    AssetStreamHandle& operator=(const AssetStreamHandle& Other);
    AssetStreamHandle& operator=(AssetStreamHandle&& Other) noexcept;

    ~AssetStreamHandle();

    Asset* Get() const { return m_Asset; }

    explicit operator bool() const { return m_Asset != nullptr; }

private:
    void Acquire(Asset* AssetPtr);
    void Release();

private:
    Asset* m_Asset = nullptr;
};


class Asset : public Object {
public:
    ARTIFACT_CLASS();
    virtual ~Asset();

protected:
    virtual void Load() = 0;
    virtual void Unload() = 0;
    virtual void Cook(class ChunkedBinary& OutChunkedBinary);
    virtual void CollectDependencies(Array<Asset*>& OutDependencies) const;
    virtual bool OnReferencedByPointer() override;
    virtual void OnUnreferencedByPointer() override;

    SharedObjectPtr<class ChunkedBinary> GetChunkedBinary() const;
    static String DisplayNameFromPath(const String& InPath);
public:
    FUNCTION(Pure)
    virtual bool IsLoaded() const;
    FUNCTION(Pure)
    bool IsMissing() const;
    AssetStreamHandle GetStreamHandle() const;
    uint32_t GetStreamReferenceCount() const { return m_StreamReferenceCount; }
    bool IsResident() const { return m_Resident; }
    bool IsPinned() const { return m_Pinned; }
    void RetainDependency(Asset* InDependency);
    FUNCTION(Pure)
    UUID GetId() const { return m_Id; }
    FUNCTION(Pure)
    virtual String GetDisplayName() const;

    /** The asset this one lives inside of, saved and deleted together with it. */
    FUNCTION(Pure)
    Asset* GetOwner() const { return m_Owner.Get(); }
    FUNCTION(Pure)
    bool IsSubAsset() const { return m_Owner.Get() != nullptr; }

    /** The content path of the file an imported asset is built from. */
    virtual String GetSourcePath() const { return String(); }
    virtual void SetSourcePath(const String& InPath) { (void)InPath; }

    virtual String SerializeToJson() const;
    virtual void DeserializeFromJson(const String& InJson);
    virtual bool SaveExternalFiles(const String& InAssetPath) const { (void)InAssetPath; return true; }
    static String GetExternalDirectory(const String& InAssetPath);

protected:
    PROPERTY()
    UUID m_Id;

    PROPERTY()
    AssetStreamType m_StreamType = AssetStreamType::Referenced;

    AssetFlags m_Flags = AssetFlags::None;
    WeakObjectPtr<Asset> m_Owner;

    uint32_t m_StreamReferenceCount = 0;
    bool m_Resident = false;
    bool m_Pinned = false;
    bool m_UnloadPending = false;
    uint64_t m_UnloadAfterFrame = 0;
    Array<AssetStreamHandle> m_Dependencies;

    void AddStreamReference();
    void RemoveStreamReference();

    friend struct AssetStreamHandle;
    friend class AssetManager;
    friend class AssetCookerEngine;
};

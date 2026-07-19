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
    virtual ~Asset() = default;

protected:
    virtual void Load() = 0;
    virtual void Unload() = 0;
    virtual void Cook(class ChunkedBinary& OutChunkedBinary);

    SharedObjectPtr<class ChunkedBinary> GetChunkedBinary() const;
    static String DisplayNameFromPath(const String& InPath);
public:
    virtual bool IsLoaded() const;
    bool IsMissing() const;
    AssetStreamHandle GetStreamHandle() const;
    UUID GetId() const { return m_Id; }
    virtual String GetDisplayName() const;

protected:
    PROPERTY()
    UUID m_Id;

    PROPERTY()
    AssetStreamType m_StreamType = AssetStreamType::Referenced;

    AssetFlags m_Flags = AssetFlags::None;

    uint32_t m_ActiveStreamHandleCount = 0;

    friend struct AssetStreamHandle;
    friend class AssetManager;
    friend class AssetCookerEngine;
};
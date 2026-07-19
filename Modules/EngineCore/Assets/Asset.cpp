#include "Asset.h"
#include "AssetManager.h"
#include "Core/EngineConfig.h"
#include "Serialization/ChunkedBinary.h"
#include "Serialization/Binary.h"
#include <filesystem>

// AssetStreamHandle

AssetStreamHandle::AssetStreamHandle(Asset* InAsset) {
    Acquire(InAsset);
}

AssetStreamHandle::AssetStreamHandle(const AssetStreamHandle& Other) {
    Acquire(Other.m_Asset);
}

AssetStreamHandle::AssetStreamHandle(AssetStreamHandle&& Other) noexcept : m_Asset(Other.m_Asset) {
    Other.m_Asset = nullptr;
}

AssetStreamHandle& AssetStreamHandle::operator=(const AssetStreamHandle& Other) {
    if (this == &Other)
        return *this;

    Release();
    Acquire(Other.m_Asset);

    return *this;
}

AssetStreamHandle& AssetStreamHandle::operator=(AssetStreamHandle&& Other) noexcept {
    if (this == &Other)
        return *this;

    Release();

    m_Asset = Other.m_Asset;
    Other.m_Asset = nullptr;

    return *this;
}

AssetStreamHandle::~AssetStreamHandle() {
    Release();
}

void AssetStreamHandle::Acquire(Asset* AssetPtr) {
    m_Asset = AssetPtr;

    if (m_Asset) {
        ++m_Asset->m_ActiveStreamHandleCount;
    }
}

void AssetStreamHandle::Release() {
    if (!m_Asset)
        return;

    AE_ASSERT(m_Asset->m_ActiveStreamHandleCount > 0);

    --m_Asset->m_ActiveStreamHandleCount;

    if (m_Asset->m_ActiveStreamHandleCount == 0) {
        AssetManager::Get().UnloadAsset(m_Asset);
    }

    m_Asset = nullptr;
}


// Asset

void Asset::Cook(ChunkedBinary& OutChunkedBinary) {
    auto bytes = BinarySerializer::SerializeObject(this);
    ChunkWriter writer;
    writer.WriteBytes(bytes->GetData(), bytes->GetSizeInBytes());
    OutChunkedBinary.AddChunk(0, writer);
}

SharedObjectPtr<ChunkedBinary> Asset::GetChunkedBinary() const {
    return ChunkedBinary::LoadFromFile(EngineConfig::ResolveContentPath("/" + GetId().ToString()));
}

bool Asset::IsLoaded() const {
    return true;
}

bool Asset::IsMissing() const {
    return (m_Flags & AssetFlags::Missing) != AssetFlags::None;
}

AssetStreamHandle Asset::GetStreamHandle() const {
    return AssetStreamHandle(const_cast<Asset*>(this));
}

String Asset::GetDisplayName() const {
    return m_Id.ToString();
}

String Asset::DisplayNameFromPath(const String& InPath) {
    return std::filesystem::path(InPath).stem().string();
}
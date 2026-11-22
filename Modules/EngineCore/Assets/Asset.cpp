#include "Asset.h"
#include "AssetManager.h"
#include "Core/EngineConfig.h"
#include "Serialization/ChunkedBinary.h"
#include "Serialization/Binary.h"
#include "Serialization/Json.h"
#include "Serialization/ThirdParty/nlohmann/json.hpp"
#include <filesystem>

// AssetStreamHandle

AssetStreamHandle::AssetStreamHandle(Asset* InAsset) {
    Acquire(InAsset);
}

AssetStreamHandle::AssetStreamHandle(const AssetStreamHandle& Other) {
    Acquire(Other.Get());
}

AssetStreamHandle::AssetStreamHandle(AssetStreamHandle&& Other) noexcept : m_Asset(std::move(Other.m_Asset)) {
}

AssetStreamHandle& AssetStreamHandle::operator=(const AssetStreamHandle& Other) {
    if (this == &Other)
        return *this;

    Release();
    Acquire(Other.Get());

    return *this;
}

AssetStreamHandle& AssetStreamHandle::operator=(AssetStreamHandle&& Other) noexcept {
    if (this == &Other)
        return *this;

    Release();

    {
        NonHoldingPointerScope nonHolding;
        m_Asset = Other.Get();
    }
    Other.m_Asset = nullptr;

    return *this;
}

Asset* AssetStreamHandle::Get() const {
    return m_Asset.Get();
}

AssetStreamHandle::operator bool() const {
    return Get() != nullptr;
}

AssetStreamHandle::~AssetStreamHandle() {
    Release();
}

void AssetStreamHandle::Acquire(Asset* AssetPtr) {
    {
        NonHoldingPointerScope nonHolding;
        m_Asset = AssetPtr;
    }
    if (AssetPtr) {
        AssetPtr->AddStreamReference();
    }
}

void AssetStreamHandle::Release() {
    Asset* asset = Get();
    if (!asset)
        return;

    m_Asset = nullptr;
    if (AssetManager::IsAvailable()) {
        asset->RemoveStreamReference();
    }
}


// Asset

Asset::~Asset() {
    if (m_UnloadPending && AssetManager::IsAvailable()) {
        AssetManager::Get().CancelUnload(this);
    }
}

void Asset::AddStreamReference() {
    if (++m_StreamReferenceCount == 1 && AssetManager::IsAvailable()) {
        AssetManager::Get().StreamIn(this);
    }
}

void Asset::RemoveStreamReference() {
    AE_ASSERT(m_StreamReferenceCount > 0);
    if (--m_StreamReferenceCount == 0 && AssetManager::IsAvailable()) {
        AssetManager::Get().RequestUnload(this);
    }
}

bool Asset::OnReferencedByPointer() {
    if (!AssetManager::IsAvailable() || AssetManager::Get().GetAsset(m_Id) != this) {
        return false;
    }
    AddStreamReference();
    return true;
}

void Asset::OnUnreferencedByPointer() {
    if (m_StreamReferenceCount > 0) {
        RemoveStreamReference();
    }
}

void Asset::RetainDependency(Asset* InDependency) {
    if (!InDependency || InDependency == this || !m_Resident) {
        return;
    }
    for (const AssetStreamHandle& handle : m_Dependencies) {
        if (handle.Get() == InDependency) {
            return;
        }
    }
    m_Dependencies.Add(InDependency->GetStreamHandle());
}

static void AddDependency(Asset* InAsset, Array<Asset*>& OutDependencies) {
    if (InAsset && !OutDependencies.Contains(InAsset)) {
        OutDependencies.Add(InAsset);
    }
}

static void CollectObjectDependencies(const Object* InObject, Array<const Object*>& InOutVisited, Array<Asset*>& OutDependencies);

static void CollectPropertyDependencies(Property* InProperty, void* InValue, Array<const Object*>& InOutVisited, Array<Asset*>& OutDependencies) {
    if (Cast<WeakObjectPtrProperty>(InProperty)) {
        AddDependency(Cast<Asset>((*(WeakObjectPtr<Object>*)InValue).Get()), OutDependencies);
    } else if (Cast<SharedObjectPtrProperty>(InProperty)) {
        Object* object = (*(SharedObjectPtr<Object>*)InValue).Get();
        if (Asset* asset = Cast<Asset>(object)) {
            AddDependency(asset, OutDependencies);
        } else {
            CollectObjectDependencies(object, InOutVisited, OutDependencies);
        }
    } else if (SubclassOfProperty* subclass = Cast<SubclassOfProperty>(InProperty)) {
        const UUID id = subclass->Read(InValue).GetBlueprintId();
        if (id.IsValid() && AssetManager::IsAvailable()) {
            AddDependency(AssetManager::Get().GetAsset(id), OutDependencies);
        }
    } else if (ArrayProperty* array = Cast<ArrayProperty>(InProperty)) {
        const size_t count = array->GetSize(InValue);
        for (size_t i = 0; i < count; i++) {
            CollectPropertyDependencies(array->InnerProperty, array->GetElementPtr(InValue, i), InOutVisited, OutDependencies);
        }
    } else if (MapProperty* map = Cast<MapProperty>(InProperty)) {
        Array<MapProperty::Entry> entries;
        map->GetEntries(InValue, entries);
        for (const MapProperty::Entry& entry : entries) {
            CollectPropertyDependencies(map->KeyProperty, (void*)entry.Key, InOutVisited, OutDependencies);
            CollectPropertyDependencies(map->ValueProperty, entry.Value, InOutVisited, OutDependencies);
        }
    } else if (StructProperty* structProperty = Cast<StructProperty>(InProperty)) {
        for (Property* inner : Property::GetTypeProperties(structProperty->InnerStructTypename)) {
            CollectPropertyDependencies(inner, inner->GetValuePtr(InValue), InOutVisited, OutDependencies);
        }
    }
}

static void CollectObjectDependencies(const Object* InObject, Array<const Object*>& InOutVisited, Array<Asset*>& OutDependencies) {
    if (!InObject || InOutVisited.Contains(InObject)) {
        return;
    }
    InOutVisited.Add(InObject);
    for (Property* property : Property::GetAllTypeProperties(InObject->GetClass())) {
        CollectPropertyDependencies(property, property->GetValuePtr((void*)InObject), InOutVisited, OutDependencies);
    }
}

void Asset::CollectDependencies(Array<Asset*>& OutDependencies) const {
    Array<const Object*> visited;
    CollectObjectDependencies(this, visited, OutDependencies);
}

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
    const String path = AssetManager::IsAvailable() ? AssetManager::Get().GetAssetPath(m_Id) : String();
    return path.empty() ? m_Id.ToString() : DisplayNameFromPath(path);
}

String Asset::SerializeToJson() const {
    nlohmann::json json = nlohmann::json::parse(JsonSerializer::SerializeObject(this));
    json["AssetClass"] = GetClass().Name;
    return json.dump(4);
}

void Asset::DeserializeFromJson(const String& InJson) {
    JsonSerializer::DeserializeObject(this, InJson);
}

String Asset::GetExternalDirectory(const String& InAssetPath) {
    const std::filesystem::path path(InAssetPath);
    return path.parent_path().string() + "/." + path.stem().string();
}

String Asset::DisplayNameFromPath(const String& InPath) {
    return std::filesystem::path(InPath).stem().string();
}

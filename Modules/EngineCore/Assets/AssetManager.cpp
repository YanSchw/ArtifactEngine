#include "AssetManager.h"
#include "Asset.h"
#include "Platform/FileIO.h"
#include "Serialization/ThirdParty/nlohmann/json.hpp"
#include "Serialization/Json.h"
#include "Core/EngineConfig.h"

#include <thread>
#include <mutex>

static AssetManager* s_Instance = nullptr;
static std::thread s_AssetLoadingThread;
static std::mutex s_AssetLoadingMutex;

struct AssetLoadRequest {
    Asset* m_Asset;
    bool m_Load; // true for load, false for unload
};

static Array<AssetLoadRequest> s_PendingAssetLoadRequests;


AssetManager& AssetManager::Get() {
    AE_ASSERT(s_Instance, "AssetManager instance is not initialized yet!");
    return *s_Instance;
}

void AssetManager::Initialize() {
    AE_ASSERT(!s_Instance, "AssetManager instance is already initialized!");
    s_Instance = this;

    // Spawn a separate thread to load assets
    s_AssetLoadingThread = std::thread(AssetManager::AssetStreamingThreadFunc);

    if (EngineConfig::IsPackagedBuild()) {
        // In packaged builds, we would load asset metadata from a file (e.g., "AssetIndex.json") that lists all assets and their UUIDs.
        AE_ASSERT(false, "Asset loading from packaged builds is not implemented yet!");
    } else {
        // In non-packaged builds, we can scan the Content directory for assets and load their metadata.
        HotLoadAssets();
    }

    // Load all Assets that are marked as AlwaysLoaded
    for (auto& [id, asset] : m_Assets) {
        if (asset->m_StreamType == AssetStreamType::AlwaysLoaded) {
            AE_INFO("Loading asset: {0}", asset->m_Id.ToString());
            asset->Load();
        }
    }
}

void AssetManager::Shutdown() {
    AE_ASSERT(s_Instance, "AssetManager instance is not initialized!");
    delete s_Instance;
    s_Instance = nullptr;
}

AssetManager::~AssetManager() {
    for (auto& [id, asset] : m_Assets) {
        AE_INFO("Unloading asset: {0}", asset->m_Id.ToString());
        asset->Unload();
    }

    s_Instance = nullptr;

    if (s_AssetLoadingThread.joinable()) {
        s_AssetLoadingThread.join();
    }
}

void AssetManager::HotLoadAssets() {
    AE_ASSERT(!EngineConfig::IsPackagedBuild(), "Hot loading is only supported in non-packaged builds!");

    Array<String> contentFiles = FileIO::ListFilesInDirectory(EngineConfig::ContentDir(), true);
    Map<Asset*, String> jsonStrings;
    for (const String& filePath : contentFiles) {
        String extension = std::filesystem::path(filePath).extension().string();
        if (extension == ".asset") {
            String jsonString = FileIO::ReadFileToString(filePath);

            // extract AssetClass and m_Id from the JSON to create the Asset instance and store the JSON string for later deserialization
            nlohmann::json j = nlohmann::json::parse(jsonString);
            AE_ASSERT(j.contains("AssetClass"), "JSON does not contain type information");
            String assetClassName = j["AssetClass"].get<String>();
            AE_ASSERT(j.contains("m_Id"), "JSON does not contain asset ID");
            UUID assetId = UUID::FromString(j["m_Id"].get<String>());

            Asset* asset = Object::Create(Class(assetClassName))->As<Asset>();
            asset->m_Id = assetId;
            jsonStrings[asset] = jsonString;
            m_Assets[asset->m_Id] = asset;
        }
    }

    for (auto& [asset, jsonString] : jsonStrings) {
        JsonSerializer::DeserializeObject(asset, jsonString);
    }
}

void AssetManager::LoadAsset(Asset* InAsset) {
    if (!InAsset)
        return;

    std::lock_guard<std::mutex> lock(s_AssetLoadingMutex);
    AE_ASSERT(false, "Not implemented yet");
}

void AssetManager::UnloadAsset(Asset* InAsset) {
    if (!InAsset)
        return;

    std::lock_guard<std::mutex> lock(s_AssetLoadingMutex);
    AE_ASSERT(false, "Not implemented yet");
}

Asset* AssetManager::GetAsset(const UUID& InId) {
    if (!m_Assets.ContainsKey(InId))
        return nullptr;

    return m_Assets.At(InId);
}

void AssetManager::AssetStreamingThreadFunc() {
    while (s_Instance) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        AssetLoadRequest request;
        {
            std::lock_guard<std::mutex> lock(s_AssetLoadingMutex);
            if (s_PendingAssetLoadRequests.IsEmpty())
                continue;

            if (s_PendingAssetLoadRequests.Size() > 256) {
                AE_WARN("{0} pending asset load requests detected. Consider optimizing asset streaming to avoid stalling the loading thread.", s_PendingAssetLoadRequests.Size());
            }

            request = s_PendingAssetLoadRequests[0];
            s_PendingAssetLoadRequests.RemoveAt(0);
        }

        AE_ASSERT(request.m_Asset, "Asset load request has null asset pointer!");
        AE_ASSERT(request.m_Asset->IsLoaded() != request.m_Load, "Asset is already in the desired load state!");

        if (request.m_Load) {
            AE_INFO("Loading asset: {0}", request.m_Asset->m_Id.ToString());
            request.m_Asset->Load();
        } else {
            AE_INFO("Unloading asset: {0}", request.m_Asset->m_Id.ToString());
            request.m_Asset->Unload();
        }

    }
}
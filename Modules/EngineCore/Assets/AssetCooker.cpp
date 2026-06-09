#include "AssetCooker.h"
#include "CoreMinimal.h"
#include "Platform/Platform.h"
#include "Platform/FileIO.h"
#include "Core/EngineConfig.h"
#include "Rendering/RenderingAPI.h"
#include "Serialization/ChunkedBinary.h"
#include "Assets/AssetManager.h"
#include "Assets/Asset.h"

void AssetCookerEngine::Initialize() {
    (new AssetManager())->Initialize(false);

    String cookDir = EngineConfig::GetConfigVar<String>("CookDirectory");
    AE_ASSERT(!cookDir.empty(), "CookedContentDir config variable is not set!");

    ChunkWriter assetIndexBinaryChunk0;
    ChunkWriter assetIndexBinaryChunk1;
    ChunkedBinary assetIndexBinary;

    Array<Asset*> assets = AssetManager::Get().GetAllAssets();
    uint32_t counter = 0;
    for (Asset* asset : assets) {
        AE_INFO("[{0}/{1}] Cooking asset: {2}", ++counter, assets.Size(), asset->GetId().ToString());
        ChunkedBinary binary;
        asset->Cook(binary);

        assetIndexBinaryChunk1 << asset->GetId();
        assetIndexBinaryChunk1 << asset->GetClass().Name;

        binary.SaveToFile(cookDir + "/" + asset->GetId().ToString());
    }

    assetIndexBinaryChunk0 << (uint32_t)assets.Size();
    assetIndexBinary.AddChunk(0, assetIndexBinaryChunk0);
    assetIndexBinary.AddChunk(1, assetIndexBinaryChunk1);
    assetIndexBinary.SaveToFile(cookDir + "/AssetIndex");
}

bool AssetCookerEngine::MainTick(double InDeltaTime) {
    return false;
}

void AssetCookerEngine::Shutdown() {
    AssetManager::Get().Shutdown();
}
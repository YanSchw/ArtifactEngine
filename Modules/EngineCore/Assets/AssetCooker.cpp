#include "AssetCooker.h"
#include "CoreMinimal.h"
#include "Platform/Platform.h"
#include "Platform/FileIO.h"
#include "Core/EngineConfig.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/ShaderLibrary.h"
#include "Serialization/ChunkedBinary.h"
#include "Assets/AssetManager.h"
#include "Assets/Asset.h"
#include "Assets/ShaderGraph.h"

void AssetCookerEngine::Initialize() {
    (new AssetManager())->Initialize(false);

    String cookDir = EngineConfig::GetConfigVar<String>("CookDirectory");
    AE_ASSERT(!cookDir.empty(), "CookDirectory config variable is not set!");

    PlatformType targetPlatform = Platform::CurrentPlatform();
    const String targetPlatformName = EngineConfig::GetConfigVar<String>("CookPlatform");
    if (!targetPlatformName.empty()) {
        targetPlatform = EPlatformType::ConvertStringToEnum(targetPlatformName);
    }

    Array<Asset*> assets;
    for (Asset* asset : AssetManager::Get().GetAllAssets()) {
        if (EngineConfig::IsPackagedContentPath(AssetManager::Get().GetAssetPath(asset->GetId()))) {
            assets.Add(asset);
        }
    }

    for (Asset* asset : assets) {
        if (ShaderGraph* graph = Cast<ShaderGraph>(asset)) {
            if (!graph->RegisterGeneratedSource()) {
                AE_ERROR("Shader graph cooking failed");
                std::exit(1);
            }
        }
    }

    if (!ShaderLibrary::Cook(cookDir, targetPlatform)) {
        AE_ERROR("Shader cooking failed");
        std::exit(1);
    }

    ChunkWriter assetIndexBinaryChunk0;
    ChunkWriter assetIndexBinaryChunk1;
    ChunkedBinary assetIndexBinary;

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
#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "Common/UUID.h"
#include "Common/Map.h"
#include "Common/Array.h"
#include "ThumbnailRenderer.gen.h"

class Asset;
struct AssetStreamHandle;
class Texture;
class World;
class RenderPipeline;
class RenderTargetTexture;

class ThumbnailRenderer : public Object {
public:
    ARTIFACT_CLASS();

    static bool CanPreview(const Asset* InAsset);
    Texture* GetThumbnail(Asset* InAsset, AssetStreamHandle& OutHold);

    void Tick();

private:
    struct Entry {
        SharedObjectPtr<RenderPipeline> Pipeline;
        SharedObjectPtr<World> Scene;
        SharedObjectPtr<RenderTargetTexture> Texture;
        int32_t Attempts = 0;
        uint64_t RenderedInFrame = 0;
        bool Rendered = false;
    };

    void ReleaseRenderedScenes();
    void RenderEntry(const UUID& InId, Entry& InEntry);

    Map<UUID, Entry> m_Cache;
    Array<UUID> m_Pending;
};

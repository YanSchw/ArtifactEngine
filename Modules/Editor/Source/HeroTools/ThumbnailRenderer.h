#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "Common/UUID.h"
#include "Common/Map.h"
#include "Common/Array.h"
#include "ThumbnailRenderer.gen.h"

class Asset;
class Texture;
class World;
class RenderPipeline;
class RenderTargetTexture;

class ThumbnailRenderer : public Object {
public:
    ARTIFACT_CLASS();

    Texture* GetThumbnail(Asset* InAsset);

    void Tick();

private:
    struct Entry {
        SharedObjectPtr<RenderPipeline> Pipeline;
        SharedObjectPtr<World> Scene;
        SharedObjectPtr<RenderTargetTexture> Texture;
        bool Rendered = false;
    };

    void RenderEntry(const UUID& InId, Entry& InEntry);

    Map<UUID, Entry> m_Cache;
    Array<UUID> m_Pending;
};

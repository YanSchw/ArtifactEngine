#pragma once
#include "GameFramework/UINode.h"
#include "Object/Object.h"
#include <functional>
#include "UIAssetSlot.gen.h"

class Asset;
class ThumbnailRenderer;

class UIAssetSlot : public UINode {
public:
    ARTIFACT_CLASS();

    Class AssetClass;
    ThumbnailRenderer* Thumbnails = nullptr;
    std::function<Asset*()> GetAsset;
    std::function<void(Asset*)> SetAsset;

    void Build();

private:
    bool Accepts(Asset* InAsset) const;
    Asset* ReadAsset() const { return GetAsset ? GetAsset() : nullptr; }
};

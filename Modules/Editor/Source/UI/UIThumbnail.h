#pragma once
#include "GameFramework/UIImage.h"
#include "Assets/Asset.h"
#include <functional>
#include "UIThumbnail.gen.h"

class UIThumbnail : public UIImage {
public:
    ARTIFACT_CLASS();

    std::function<Texture*(AssetStreamHandle&)> Resolve;
    UINode* Fallback = nullptr;

    virtual void OnUIUpdate(const UIFrameContext& InContext) override;
    virtual void Paint(UIDrawList& OutDrawList) override;

private:
    bool IsOnScreen();

    AssetStreamHandle m_Held;
};

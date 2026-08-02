#pragma once
#include "GameFramework/UINode.h"
#include "Object/Pointer.h"
#include "Common/String.h"
#include <functional>
#include "EditorDragDrop.gen.h"

class Asset;
class Texture;

/** Attach one to any node to make it take asset drops. */
class UIAssetDropZone : public UINode {
public:
    ARTIFACT_CLASS();

    UIAssetDropZone();

    std::function<bool(Asset*)> Accepts;
    std::function<void(Asset*, const Vec2&)> Dropped;

    bool CanAccept(Asset* InAsset) const { return InAsset && Accepts && Accepts(InAsset); }
};

class UIDragGhost : public UINode {
public:
    ARTIFACT_CLASS();

    UIDragGhost();

    WeakObjectPtr<Asset> DraggedAsset;
    WeakObjectPtr<Texture> Thumbnail;
    String Caption;
    Vec2 CursorPos = Vec2(0.0f);
    WeakObjectPtr<UIAssetDropZone> Target;

    virtual void PaintOverlay(UIDrawList& OutDrawList) override;
};

class EditorDragDrop {
public:
    static UIAssetDropZone* AddDropZone(UINode& InHost, std::function<bool(Asset*)> InAccepts,
                                        std::function<void(Asset*, const Vec2&)> InDropped);

    static void Begin(UINode& InSource, Asset* InAsset, Texture* InThumbnail, const String& InCaption);
    static void Update(const Vec2& InCursorPos);
    static void Drop();
    static void Cancel();

    static bool IsActive();
    static Asset* GetAsset();
};

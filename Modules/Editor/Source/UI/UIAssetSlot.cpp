#include "UIAssetSlot.h"
#include "EditorStyle.h"
#include "EditorIcons.h"
#include "UIDropdown.h"
#include "EditorDragDrop.h"
#include "HeroTools/ThumbnailRenderer.h"
#include "GameFramework/UIQuad.h"
#include "GameFramework/UISvg.h"
#include "GameFramework/UIImage.h"
#include "Assets/AssetManager.h"
#include "Assets/Asset.h"
#include "Rendering/Texture.h"
#include <memory>

static constexpr float s_ThumbnailSize = 44.0f;
static constexpr float s_DropdownGap = 6.0f;

void UIAssetSlot::Build() {
    UIQuad* frame = Add<UIQuad>();
    frame->Anchor = frame->Pivot = Vec2(0.0f, 0.5f);
    frame->Size = Vec2(s_ThumbnailSize, s_ThumbnailSize);
    frame->Color = EditorStyle::FieldBorder;

    UIQuad* background = frame->Add<UIQuad>();
    background->Anchor = background->Pivot = Vec2(0.0f);
    background->Position = Vec2(1.0f, 1.0f);
    background->Size = Vec2(s_ThumbnailSize - 2.0f, s_ThumbnailSize - 2.0f);
    background->Color = EditorStyle::PanelDark;

    UISvg* icon = background->Add<UISvg>();
    icon->Center(Vec2(22.0f, 22.0f));

    UIImage* preview = background->Add<UIImage>();
    preview->Center({ 1.0_rel - 4.0_px, 1.0_rel - 4.0_px });

    UIQuad* typeBar = frame->Add<UIQuad>();
    typeBar->Anchor = typeBar->Pivot = Vec2(0.0f, 1.0f);
    typeBar->Size = { 1.0_rel, 3.0_px };

    frame->Bind = [this, icon, preview, typeBar] {
        Asset* asset = ReadAsset();
        const Class assetClass = asset ? asset->GetClass() : AssetClass;
        const Vec4 color = EditorIcons::GetAssetColor(assetClass);

        typeBar->Color = asset ? color : Vec4(color.r, color.g, color.b, 0.35f);
        icon->Image = EditorIcons::GetAssetIcon(assetClass);
        icon->Tint = asset ? color : Vec4(color.r, color.g, color.b, 0.4f);

        Texture* thumbnail = (asset && Thumbnails) ? Thumbnails->GetThumbnail(asset) : nullptr;
        const bool ready = thumbnail && thumbnail->GetDefaultView().Get();
        preview->Image = ready ? thumbnail : nullptr;
        preview->SetEnabled(ready);
        icon->SetEnabled(!ready);
    };

    // Rebuilt on every open, so newly imported or created assets show up without a refresh.
    auto options = std::make_shared<Array<WeakObjectPtr<Asset>>>();

    UIDropdown* dropdown = Add<UIDropdown>();
    dropdown->Anchor = dropdown->Pivot = Vec2(0.0f, 0.5f);
    dropdown->Position = Vec2(s_ThumbnailSize + s_DropdownGap, 0.0f);
    dropdown->Size = { 1.0_rel - UIValue::Px(s_ThumbnailSize + s_DropdownGap), 22.0_px };
    dropdown->GetSelectedLabel = [this] {
        Asset* asset = ReadAsset();
        return asset ? asset->GetDisplayName() : String("None");
    };
    dropdown->GetOptions = [this, options] {
        options->Clear();
        Array<String> labels;
        labels.Add("None");
        for (Asset* asset : AssetManager::Get().GetAssetsOfClass(AssetClass)) {
            options->Add(WeakObjectPtr<Asset>(asset));
            labels.Add(asset->GetDisplayName());
        }
        return labels;
    };
    dropdown->GetSelectedIndex = [this, options]() -> int32_t {
        Asset* current = ReadAsset();
        for (int32_t i = 0; i < options->Size(); i++) {
            if ((*options)[i].Get() == current) {
                return i + 1;
            }
        }
        return 0;
    };
    dropdown->SelectionChanged = [this, options](int32_t InIndex) {
        Asset* asset = (InIndex > 0 && InIndex <= options->Size()) ? (*options)[InIndex - 1].Get() : nullptr;
        if (SetAsset) {
            SetAsset(asset);
        }
    };

    EditorDragDrop::AddDropZone(*this,
        [this](Asset* InAsset) { return Accepts(InAsset); },
        [this](Asset* InAsset, const Vec2&) { SetAsset(InAsset); });
}

bool UIAssetSlot::Accepts(Asset* InAsset) const {
    return InAsset && SetAsset && InAsset->IsA(AssetClass);
}

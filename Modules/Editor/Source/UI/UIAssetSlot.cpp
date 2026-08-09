#include "UIAssetSlot.h"
#include "EditorStyle.h"
#include "EditorIcons.h"
#include "UIDropdown.h"
#include "EditorDragDrop.h"
#include "EditorWindow.h"
#include "HeroTools/ContentDrawer.h"
#include "HeroTools/ThumbnailRenderer.h"
#include "GameFramework/UIQuad.h"
#include "GameFramework/UISvg.h"
#include "GameFramework/UIImage.h"
#include "GameFramework/UIButton.h"
#include "Assets/AssetManager.h"
#include "Assets/Asset.h"
#include "Rendering/Texture.h"
#include <memory>

static constexpr float s_ThumbnailSize = 44.0f;
static constexpr float s_ColumnGap = 6.0f;
static constexpr float s_ColumnLeft = s_ThumbnailSize + s_ColumnGap;
static constexpr float s_DropdownHeight = 22.0f;
static constexpr float s_ActionButtonSize = 20.0f;

static const Vec4 s_DisabledIcon = HexColor(0x5C5C5C);

void UIAssetSlot::Build() {
    BuildThumbnail();
    BuildDropdown();
    BuildActionButtons();

    EditorDragDrop::AddDropZone(*this,
        [this](Asset* InAsset) { return Accepts(InAsset); },
        [this](Asset* InAsset, const Vec2&) { SetAsset(InAsset); });
}

void UIAssetSlot::BuildThumbnail() {
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
}

void UIAssetSlot::BuildDropdown() {
    // Rebuilt on every open, so newly imported or created assets show up without a refresh.
    auto options = std::make_shared<Array<WeakObjectPtr<Asset>>>();

    UIDropdown* dropdown = Add<UIDropdown>();
    dropdown->Anchor = dropdown->Pivot = Vec2(0.0f, 0.5f);
    dropdown->Position = Vec2(s_ColumnLeft, -0.5f * (s_ThumbnailSize - s_DropdownHeight));
    dropdown->Size = { 1.0_rel - UIValue::Px(s_ColumnLeft), UIValue(s_DropdownHeight) };
    dropdown->SearchPlaceholder = "Search " + (AssetClass == Class::None ? String("Assets") : AssetClass.GetDisplayName());
    dropdown->GetSelectedLabel = [this] {
        Asset* asset = ReadAsset();
        return asset ? asset->GetDisplayName() : String("None");
    };
    dropdown->GetOptions = [this, options] {
        options->Clear();
        Array<UIDropdownOption> entries;
        entries.Add(UIDropdownOption("None"));
        for (Asset* asset : AssetManager::Get().GetAssetsOfClass(AssetClass)) {
            options->Add(WeakObjectPtr<Asset>(asset));

            UIDropdownOption entry(asset->GetDisplayName());
            entry.Icon = EditorIcons::GetAssetIcon(asset->GetClass());
            entry.IconTint = EditorIcons::GetAssetColor(asset->GetClass());
            entry.Thumbnail = Thumbnails ? Thumbnails->GetThumbnail(asset) : nullptr;
            entries.Add(entry);
        }
        return entries;
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
}

void UIAssetSlot::BuildActionButtons() {
    const auto addButton = [this](float InX, VectorImage* InIcon, std::function<Asset*()> InTarget,
                                  std::function<void(Asset*)> InAction) {
        UIButton* button = Add<UIButton>();
        button->Anchor = button->Pivot = Vec2(0.0f, 0.5f);
        button->Position = Vec2(InX, 0.5f * (s_ThumbnailSize - s_ActionButtonSize));
        button->Size = Vec2(s_ActionButtonSize, s_ActionButtonSize);
        button->NormalColor = EditorStyle::Button;
        button->HoverColor = EditorStyle::ButtonHover;
        button->PressedColor = EditorStyle::ButtonPressed;
        button->Clicked = [InTarget, InAction] {
            if (Asset* target = InTarget()) {
                InAction(target);
            }
        };

        UISvg* icon = button->Add<UISvg>();
        icon->Center(Vec2(13.0f, 13.0f));
        icon->Image = InIcon;

        button->Bind = [button, icon, InTarget] {
            const bool available = InTarget() != nullptr;
            button->Interactable = available;
            button->Cursor = available ? CursorIcon::Hand : CursorIcon::Arrow;
            icon->Tint = available ? EditorStyle::Text : s_DisabledIcon;
        };
    };

    addButton(s_ColumnLeft, EditorIcons::UseSelected(),
        [this]() -> Asset* {
            ContentDrawer* drawer = GetContentDrawer();
            Asset* selected = drawer ? drawer->GetSelectedAsset() : nullptr;
            return Accepts(selected) ? selected : nullptr;
        },
        [this](Asset* InAsset) { SetAsset(InAsset); });

    addButton(s_ColumnLeft + s_ActionButtonSize + 2.0f, EditorIcons::Search(),
        [this] { return ReadAsset(); },
        [this](Asset* InAsset) {
            EditorWindow* window = EditorWindow::FindFor(*this);
            ContentDrawer* drawer = window ? window->GetContentDrawer() : nullptr;
            if (!drawer) {
                return;
            }
            window->OpenHeroTool(drawer);
            drawer->RevealAsset(InAsset);
        });
}

ContentDrawer* UIAssetSlot::GetContentDrawer() const {
    EditorWindow* window = EditorWindow::FindFor(*this);
    return window ? window->GetContentDrawer() : nullptr;
}

bool UIAssetSlot::Accepts(Asset* InAsset) const {
    return InAsset && SetAsset && InAsset->IsA(AssetClass);
}

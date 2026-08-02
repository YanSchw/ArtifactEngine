#include "EditorDragDrop.h"
#include "EditorStyle.h"
#include "EditorIcons.h"
#include "GameFramework/UICanvas.h"
#include "Assets/Asset.h"
#include "Assets/Font.h"
#include "Rendering/Texture.h"
#include "Rendering/UIDrawList.h"
#include <algorithm>

static constexpr float s_GhostHeight = 46.0f;
static constexpr float s_GhostPreview = 38.0f;
static constexpr float s_GhostPadding = 4.0f;
static constexpr float s_GhostGap = 8.0f;
static constexpr float s_CursorOffset = 14.0f;
static constexpr float s_TargetOutline = 2.0f;

static WeakObjectPtr<UIDragGhost> s_Ghost;

UIAssetDropZone::UIAssetDropZone() {
    Fill();
    Interactable = false;
}

UIDragGhost::UIDragGhost() {
    // Spans the canvas so the card can be kept inside it, but never takes part in input.
    Fill();
    Interactable = false;
}

// Deepest node under the point in paint order. The input router does the same walk, but it only
// ever delivers to the node that captured the pointer, which during a drag is the drag source.
static UINode* TopmostAt(UINode& InNode, const Vec2& InPoint) {
    if (!InNode.IsEnabled()) {
        return nullptr;
    }
    if (!InNode.ClipChildren || InNode.HitTest(InPoint)) {
        for (int32_t i = (int32_t)InNode.GetChildCount() - 1; i >= 0; i--) {
            if (UINode* child = InNode.GetChild(i)->As<UINode>()) {
                if (UINode* hit = TopmostAt(*child, InPoint)) {
                    return hit;
                }
            }
        }
    }
    return InNode.HitTest(InPoint) ? &InNode : nullptr;
}

void UIDragGhost::PaintOverlay(UIDrawList& OutDrawList) {
    Asset* asset = DraggedAsset.Get();
    if (!asset) {
        return;
    }

    if (UIAssetDropZone* target = Target.Get()) {
        const UIRectF rect = target->GetGeometry();
        const Mat4& transform = target->GetWorldMatrix();
        const Vec2 min = rect.Min();
        const Vec2 max = rect.Max();
        OutDrawList.AddRect(UIRectF(min, Vec2(rect.Size.x, s_TargetOutline)), EditorStyle::AccentBright, transform);
        OutDrawList.AddRect(UIRectF(Vec2(min.x, max.y - s_TargetOutline), Vec2(rect.Size.x, s_TargetOutline)),
                            EditorStyle::AccentBright, transform);
        OutDrawList.AddRect(UIRectF(min, Vec2(s_TargetOutline, rect.Size.y)), EditorStyle::AccentBright, transform);
        OutDrawList.AddRect(UIRectF(Vec2(max.x - s_TargetOutline, min.y), Vec2(s_TargetOutline, rect.Size.y)),
                            EditorStyle::AccentBright, transform);
    }

    Font* font = GetDefaultFont();
    const float captionWidth = font ? font->GetTextWidth(Caption, EditorStyle::FontSize) : 60.0f;
    const Vec2 size(s_GhostPadding * 2.0f + s_GhostPreview + s_GhostGap + captionWidth + s_GhostGap, s_GhostHeight);

    // The card hangs below-right of the cursor, sliding back onto the canvas at the edges.
    Vec2 position = CursorPos + Vec2(s_CursorOffset, s_CursorOffset);
    position.x = std::min(position.x, m_Geometry.Max().x - size.x - 4.0f);
    position.y = std::min(position.y, m_Geometry.Max().y - size.y - 4.0f);
    const UIRectF card(position, size);

    OutDrawList.AddRoundedRect(card, EditorStyle::Border, 5.0f);
    OutDrawList.AddRoundedRect(card.Deflate(UIPadding(1.0f)), EditorStyle::ToolBar, 4.0f);

    const UIRectF preview(position + Vec2(s_GhostPadding, (s_GhostHeight - s_GhostPreview) * 0.5f),
                          Vec2(s_GhostPreview, s_GhostPreview));
    const Vec4 typeColor = EditorIcons::GetAssetColor(asset->GetClass());
    OutDrawList.AddRect(preview, EditorStyle::PanelDark);

    Texture* thumbnail = Thumbnail.Get();
    if (thumbnail && thumbnail->GetDefaultView().Get()) {
        OutDrawList.AddImageRect(preview.Deflate(UIPadding(2.0f)), Vec4(1.0f), thumbnail);
    } else {
        EditorIcons::Paint(OutDrawList, EditorIcons::GetAssetIcon(asset->GetClass()),
                           preview.Deflate(UIPadding(8.0f)), typeColor, Mat4(1.0f));
    }

    if (font) {
        const Vec2 textSize = font->MeasureText(Caption, EditorStyle::FontSize);
        const Vec2 textPosition(preview.Max().x + s_GhostGap, position.y + (s_GhostHeight - textSize.y) * 0.5f);
        OutDrawList.AddText(font, Caption, textPosition, EditorStyle::FontSize, EditorStyle::TextBright);
    }
}

UIAssetDropZone* EditorDragDrop::AddDropZone(UINode& InHost, std::function<bool(Asset*)> InAccepts,
                                             std::function<void(Asset*, const Vec2&)> InDropped) {
    UIAssetDropZone* zone = InHost.Add<UIAssetDropZone>();
    zone->Accepts = std::move(InAccepts);
    zone->Dropped = std::move(InDropped);
    return zone;
}

void EditorDragDrop::Begin(UINode& InSource, Asset* InAsset, Texture* InThumbnail, const String& InCaption) {
    Cancel();
    UICanvas* canvas = InSource.GetCanvas();
    if (!canvas || !InAsset) {
        return;
    }

    UIDragGhost* ghost = canvas->Add<UIDragGhost>();
    ghost->DraggedAsset = InAsset;
    ghost->Thumbnail = InThumbnail;
    ghost->Caption = InCaption;
    s_Ghost = ghost;
}

void EditorDragDrop::Update(const Vec2& InCursorPos) {
    UIDragGhost* ghost = s_Ghost.Get();
    Asset* asset = ghost ? ghost->DraggedAsset.Get() : nullptr;
    if (!asset) {
        return;
    }
    ghost->CursorPos = InCursorPos;
    ghost->Target = nullptr;

    UICanvas* canvas = ghost->GetCanvas();
    if (!canvas) {
        return;
    }
    // The ghost covers the canvas, so it has to step aside for the pick.
    ghost->SetEnabled(false);
    Node* hit = TopmostAt(*canvas, InCursorPos);
    ghost->SetEnabled(true);

    for (Node* node = hit; node; node = node->GetParent()) {
        UIAssetDropZone* zone = node->As<UIAssetDropZone>();
        if (zone && zone->CanAccept(asset)) {
            ghost->Target = zone;
            break;
        }
    }
}

void EditorDragDrop::Drop() {
    UIDragGhost* ghost = s_Ghost.Get();
    if (!ghost) {
        return;
    }
    UIAssetDropZone* target = ghost->Target.Get();
    Asset* asset = ghost->DraggedAsset.Get();
    const Vec2 cursor = ghost->CursorPos;
    Cancel();

    if (target && asset && target->Dropped) {
        target->Dropped(asset, cursor);
    }
}

void EditorDragDrop::Cancel() {
    UIDragGhost* ghost = s_Ghost.Get();
    s_Ghost = nullptr;
    if (!ghost) {
        return;
    }
    if (UICanvas* canvas = ghost->GetCanvas()) {
        canvas->DestroyDeferred(ghost);
    }
}

bool EditorDragDrop::IsActive() {
    UIDragGhost* ghost = s_Ghost.Get();
    return ghost && ghost->DraggedAsset.Get() != nullptr;
}

Asset* EditorDragDrop::GetAsset() {
    UIDragGhost* ghost = s_Ghost.Get();
    return ghost ? ghost->DraggedAsset.Get() : nullptr;
}

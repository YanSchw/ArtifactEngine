#include "UIDockArea.h"
#include "EditorStyle.h"
#include "Tabs/MinorTab.h"
#include "Tabs/MajorTab.h"
#include "EditorWindow.h"
#include "Assets/Font.h"
#include "Rendering/UIDrawList.h"

UIDockArea::UIDockArea() {
    Fill();
    m_Root = Add<UIDockNode>();
}

UIDockNode* UIDockArea::Dock(MinorTab* InTab, UIDockSlot InSlot, UIDockNode* InTarget, float InShare) {
    if (MajorTab* owner = GetParent() ? GetParent()->As<MajorTab>() : nullptr) {
        InTab->SetEditedWorld(owner->GetEditedWorld());
    }
    UIDockNode* target = InTarget ? InTarget : m_Root;
    if (InSlot == UIDockSlot::Center) {
        target = target->FindFirstLeaf();
        target->AddTab(InTab);
        return target;
    }
    if (target->IsLeaf() && target->GetTabs().IsEmpty()) {
        target->AddTab(InTab);
        return target;
    }
    UIDockNode* leaf = Object::Create<UIDockNode>();
    leaf->AddTab(InTab);
    target->BecomeSplit(InSlot, leaf, InShare);
    return leaf;
}

void UIDockArea::BeginTabDrag(MinorTab* InTab, UIDockNode* InSource, const Vec2& InCursorPos) {
    m_DraggedTab = InTab;
    m_DragSource = InSource;
    m_DragCursor = InCursorPos;
}

void UIDockArea::UpdateTabDrag(const Vec2& InCursorPos) {
    m_DragCursor = InCursorPos;
}

void UIDockArea::EndTabDrag() {
    MinorTab* tab = m_DraggedTab;
    UIDockNode* source = m_DragSource;
    m_DraggedTab = nullptr;
    m_DragSource = nullptr;
    if (!tab || !source) {
        return;
    }

    UIDockNode* target = nullptr;
    UIDockSlot slot = UIDockSlot::Center;
    UIRectF preview;
    if (!ResolveDropTarget(m_DragCursor, target, slot, preview)) {
        if (!GetGeometry().Contains(m_DragCursor)) {
            FloatDraggedTab(tab, source);
        }
        return;
    }
    if (target == source && source->GetTabs().Size() == 1) {
        return;
    }

    DetachTab(tab, source, target);
    Dock(tab, slot, target, 0.5f);
}

void UIDockArea::DetachTab(MinorTab* InTab, UIDockNode* InSource, UIDockNode*& InOutTarget) {
    InSource->RemoveTab(InTab);
    if (!InSource->GetTabs().IsEmpty()) {
        return;
    }
    // Collapse the emptied leaf: its parent split absorbs the sibling. The absorb deletes
    // the sibling's shell, so a drop targeted at it is retargeted to the surviving parent.
    if (UIDockNode* parent = InSource->GetParent() ? InSource->GetParent()->As<UIDockNode>() : nullptr) {
        UIDockNode* kept = (parent->GetChildA() == InSource) ? parent->GetChildB() : parent->GetChildA();
        parent->AbsorbChild(kept);
        if (InOutTarget == kept || InOutTarget == InSource) {
            InOutTarget = parent;
        }
    }
}

void UIDockArea::FloatDraggedTab(MinorTab* InTab, UIDockNode* InSource) {
    MajorTab* majorTab = nullptr;
    for (Node* node = GetParent(); node; node = node->GetParent()) {
        if ((majorTab = node->As<MajorTab>())) {
            break;
        }
    }
    if (!majorTab || !majorTab->GetOwnerWindow()) {
        return;
    }

    UIDockNode* unusedTarget = nullptr;
    DetachTab(InTab, InSource, unusedTarget);
    const Vec2 screenPos = majorTab->GetOwnerWindow()->GetPosition() + m_DragCursor;
    majorTab->FloatTab(InTab, screenPos);
}

bool UIDockArea::ResolveDropTarget(const Vec2& InPoint, UIDockNode*& OutNode, UIDockSlot& OutSlot, UIRectF& OutPreview) const {
    UIDockNode* leaf = m_Root ? m_Root->FindLeafAt(InPoint) : nullptr;
    if (!leaf) {
        return false;
    }
    OutNode = leaf;

    const UIRectF rect = leaf->GetGeometry();
    const Vec2 local = (InPoint - rect.Position) / rect.Size;

    if (leaf->GetHeaderRect().Contains(InPoint) || (std::abs(local.x - 0.5f) < 0.25f && std::abs(local.y - 0.5f) < 0.25f)) {
        OutSlot = UIDockSlot::Center;
        OutPreview = rect;
        return true;
    }

    // Outside the center zone the dominant axis picks the edge.
    const Vec2 fromCenter = local - Vec2(0.5f);
    if (std::abs(fromCenter.x) > std::abs(fromCenter.y)) {
        OutSlot = (fromCenter.x < 0.0f) ? UIDockSlot::Left : UIDockSlot::Right;
    } else {
        OutSlot = (fromCenter.y < 0.0f) ? UIDockSlot::Top : UIDockSlot::Bottom;
    }

    OutPreview = rect;
    switch (OutSlot) {
        case UIDockSlot::Left: OutPreview.Size.x *= 0.5f; break;
        case UIDockSlot::Right: OutPreview.Size.x *= 0.5f; OutPreview.Position.x += OutPreview.Size.x; break;
        case UIDockSlot::Top: OutPreview.Size.y *= 0.5f; break;
        case UIDockSlot::Bottom: OutPreview.Size.y *= 0.5f; OutPreview.Position.y += OutPreview.Size.y; break;
        default: break;
    }
    return true;
}

void UIDockArea::PaintOverlay(UIDrawList& OutDrawList) {
    if (!m_DraggedTab) {
        return;
    }

    UIDockNode* target = nullptr;
    UIDockSlot slot = UIDockSlot::Center;
    UIRectF preview;
    if (ResolveDropTarget(m_DragCursor, target, slot, preview)) {
        OutDrawList.AddRect(preview, EditorStyle::DropPreview, m_WorldMatrix);
    }

    // Ghost handle following the cursor.
    if (Font* font = GetDefaultFont()) {
        const String title = m_DraggedTab->GetTabTitle();
        const Vec2 textSize = font->MeasureText(title, EditorStyle::FontSize);
        const UIRectF ghost(m_DragCursor + Vec2(12.0f, 8.0f), Vec2(textSize.x + 24.0f, EditorStyle::TabHeaderHeight));
        OutDrawList.AddRect(ghost, EditorStyle::TabHover, m_WorldMatrix);
        OutDrawList.AddText(font, title, ghost.Center() - textSize * 0.5f, EditorStyle::FontSize, EditorStyle::TextBright, m_WorldMatrix);
    }
}

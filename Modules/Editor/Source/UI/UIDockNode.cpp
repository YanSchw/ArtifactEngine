#include "UIDockNode.h"
#include "UIDockArea.h"
#include "UIDockSplitter.h"
#include "EditorStyle.h"
#include "Tabs/MinorTab.h"
#include "Assets/Font.h"
#include "Rendering/UIDrawList.h"
#include <algorithm>

UIDockNode::UIDockNode() {
    Fill();
    Interactable = true;
}

void UIDockNode::AddTab(MinorTab* InTab, bool InActivate) {
    InTab->SetParent(this);
    m_Tabs.Add(InTab);
    if (InActivate || m_Tabs.Size() == 1) {
        SetActiveTab(InTab);
    } else {
        InTab->SetEnabled(false);
    }
}

void UIDockNode::RemoveTab(MinorTab* InTab) {
    if (!m_Tabs.Contains(InTab)) {
        return;
    }
    m_Tabs.Remove(InTab);
    InTab->SetParent(nullptr);
    if (m_ActiveTab == InTab) {
        SetActiveTab(m_Tabs.IsEmpty() ? nullptr : m_Tabs[0]);
    }
}

void UIDockNode::SetActiveTab(MinorTab* InTab) {
    m_ActiveTab = InTab;
    for (MinorTab* tab : m_Tabs) {
        tab->SetEnabled(tab == InTab);
    }
}

void UIDockNode::BecomeSplit(UIDockSlot InSlot, UIDockNode* InInserted, float InInsertedShare) {
    UIDockNode* moved = Add<UIDockNode>();
    if (IsLeaf()) {
        for (MinorTab* tab : m_Tabs) {
            tab->SetParent(moved);
            moved->m_Tabs.Add(tab);
        }
        moved->m_ActiveTab = m_ActiveTab;
        m_Tabs.Clear();
        m_ActiveTab = nullptr;
    } else {
        moved->m_ChildA = m_ChildA;
        moved->m_ChildB = m_ChildB;
        moved->m_Splitter = m_Splitter;
        moved->m_SplitHorizontal = m_SplitHorizontal;
        moved->m_SplitRatio = m_SplitRatio;
        m_ChildA->SetParent(moved);
        m_ChildB->SetParent(moved);
        m_Splitter->SetParent(moved);
        m_ChildA = m_ChildB = nullptr;
        m_Splitter = nullptr;
    }

    InInserted->SetParent(this);
    m_Splitter = Add<UIDockSplitter>();
    m_SplitHorizontal = (InSlot == UIDockSlot::Left || InSlot == UIDockSlot::Right);
    const bool insertedFirst = (InSlot == UIDockSlot::Left || InSlot == UIDockSlot::Top);
    m_ChildA = insertedFirst ? InInserted : moved;
    m_ChildB = insertedFirst ? moved : InInserted;
    m_SplitRatio = insertedFirst ? InInsertedShare : 1.0f - InInsertedShare;
}

void UIDockNode::AbsorbChild(UIDockNode* InKeep) {
    UIDockNode* dropped = (InKeep == m_ChildA) ? m_ChildB : m_ChildA;
    UIDockSplitter* splitter = m_Splitter;
    m_ChildA = m_ChildB = nullptr;
    m_Splitter = nullptr;

    if (InKeep->IsLeaf()) {
        for (MinorTab* tab : InKeep->m_Tabs) {
            tab->SetParent(this);
            m_Tabs.Add(tab);
        }
        m_ActiveTab = InKeep->m_ActiveTab;
        InKeep->m_Tabs.Clear();
        InKeep->m_ActiveTab = nullptr;
    } else {
        m_ChildA = InKeep->m_ChildA;
        m_ChildB = InKeep->m_ChildB;
        m_Splitter = InKeep->m_Splitter;
        m_SplitHorizontal = InKeep->m_SplitHorizontal;
        m_SplitRatio = InKeep->m_SplitRatio;
        m_ChildA->SetParent(this);
        m_ChildB->SetParent(this);
        m_Splitter->SetParent(this);
        InKeep->m_ChildA = InKeep->m_ChildB = nullptr;
        InKeep->m_Splitter = nullptr;
    }

    delete dropped;
    delete splitter;
    delete InKeep;
}

void UIDockNode::AdjustSplit(const Vec2& InCursorDelta) {
    const UIRectF content = GetContentRect();
    const float extent = (m_SplitHorizontal ? content.Size.x : content.Size.y) - EditorStyle::SplitterThickness;
    if (extent <= EditorStyle::MinPanelSize * 2.0f) {
        return;
    }
    float first = m_SplitRatio * extent + (m_SplitHorizontal ? InCursorDelta.x : InCursorDelta.y);
    first = std::clamp(first, EditorStyle::MinPanelSize, extent - EditorStyle::MinPanelSize);
    m_SplitRatio = first / extent;
}

UIDockNode* UIDockNode::FindLeafAt(const Vec2& InPoint) {
    if (!GetGeometry().Contains(InPoint)) {
        return nullptr;
    }
    if (IsLeaf()) {
        return this;
    }
    if (UIDockNode* hit = m_ChildA->FindLeafAt(InPoint)) {
        return hit;
    }
    return m_ChildB->FindLeafAt(InPoint);
}

UIRectF UIDockNode::GetHeaderRect() const {
    return UIRectF(m_Geometry.Position, Vec2(m_Geometry.Size.x, EditorStyle::TabHeaderHeight));
}

float UIDockNode::TabHandleWidth(int InTabIndex) const {
    if (Font* font = GetDefaultFont()) {
        return font->MeasureText(m_Tabs[InTabIndex]->GetTabTitle(), EditorStyle::FontSize).x + 24.0f;
    }
    return 90.0f;
}

UIRectF UIDockNode::TabHandleRect(int InTabIndex) const {
    float x = m_Geometry.Position.x;
    for (int i = 0; i < InTabIndex; i++) {
        x += TabHandleWidth(i) + 1.0f;
    }
    return UIRectF(Vec2(x, m_Geometry.Position.y), Vec2(TabHandleWidth(InTabIndex), EditorStyle::TabHeaderHeight));
}

int UIDockNode::TabHandleIndexAt(const Vec2& InPoint) const {
    for (int i = 0; i < m_Tabs.Size(); i++) {
        if (TabHandleRect(i).Contains(InPoint)) {
            return i;
        }
    }
    return -1;
}

void UIDockNode::LayoutChildren(const UIRectF& InContent) {
    if (IsLeaf()) {
        UIRectF body = InContent;
        body.Position.y += EditorStyle::TabHeaderHeight;
        body.Size.y = std::max(0.0f, body.Size.y - EditorStyle::TabHeaderHeight);
        for (UINode* child : CollectUIChildren()) {
            LayoutChild(*child, body, m_WorldMatrix);
        }
        return;
    }

    const bool horizontal = m_SplitHorizontal;
    const float extent = (horizontal ? InContent.Size.x : InContent.Size.y) - EditorStyle::SplitterThickness;
    const float first = std::floor(std::max(0.0f, extent) * m_SplitRatio);
    const float second = std::max(0.0f, extent - first);

    UIRectF rectA = InContent;
    UIRectF rectSplitter = InContent;
    UIRectF rectB = InContent;
    if (horizontal) {
        rectA.Size.x = first;
        rectSplitter.Position.x += first;
        rectSplitter.Size.x = EditorStyle::SplitterThickness;
        rectB.Position.x += first + EditorStyle::SplitterThickness;
        rectB.Size.x = second;
    } else {
        rectA.Size.y = first;
        rectSplitter.Position.y += first;
        rectSplitter.Size.y = EditorStyle::SplitterThickness;
        rectB.Position.y += first + EditorStyle::SplitterThickness;
        rectB.Size.y = second;
    }
    LayoutChild(*m_ChildA, rectA, m_WorldMatrix);
    LayoutChild(*m_Splitter, rectSplitter, m_WorldMatrix);
    LayoutChild(*m_ChildB, rectB, m_WorldMatrix);
}

void UIDockNode::Paint(UIDrawList& OutDrawList) {
    if (!IsLeaf()) {
        return;
    }

    UIRectF body = m_Geometry;
    body.Position.y += EditorStyle::TabHeaderHeight;
    body.Size.y = std::max(0.0f, body.Size.y - EditorStyle::TabHeaderHeight);
    if (!(m_ActiveTab && m_ActiveTab->HasTransparentBackground())) {
        OutDrawList.AddRect(body, EditorStyle::Panel, m_WorldMatrix);
    }
    OutDrawList.AddRect(GetHeaderRect(), EditorStyle::TabBar, m_WorldMatrix);

    Font* font = GetDefaultFont();
    for (int i = 0; i < m_Tabs.Size(); i++) {
        MinorTab* tab = m_Tabs[i];
        const UIRectF handle = TabHandleRect(i);
        const bool active = (tab == m_ActiveTab);
        const bool hovered = IsHovered() && handle.Contains(m_LastCursor);
        const Vec4 background = active ? EditorStyle::TabActive : (hovered ? EditorStyle::TabHover : EditorStyle::TabBar);
        OutDrawList.AddRect(handle, background, m_WorldMatrix);
        if (font) {
            const String title = tab->GetTabTitle();
            const Vec2 textSize = font->MeasureText(title, EditorStyle::FontSize);
            const Vec2 textPos = handle.Center() - textSize * 0.5f;
            OutDrawList.AddText(font, title, textPos, EditorStyle::FontSize, active ? EditorStyle::Text : EditorStyle::TextDim, m_WorldMatrix);
        }
    }
}

void UIDockNode::OnUIUpdate(const UIFrameContext& InContext) {
    m_LastCursor = InContext.CursorPosition;
}

UIDockArea* UIDockNode::GetArea() const {
    for (Node* node = GetParent(); node; node = node->GetParent()) {
        if (UIDockArea* area = node->As<UIDockArea>()) {
            return area;
        }
    }
    return nullptr;
}

void UIDockNode::OnPressed(const Vec2& InCursorPos) {
    m_PressedTabIndex = TabHandleIndexAt(InCursorPos);
    m_PressPosition = InCursorPos;
    if (m_PressedTabIndex >= 0) {
        SetActiveTab(m_Tabs[m_PressedTabIndex]);
    }
}

void UIDockNode::OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) {
    (void)InDelta;
    if (m_PressedTabIndex < 0) {
        return;
    }
    UIDockArea* area = GetArea();
    if (!area) {
        return;
    }
    if (area->IsDraggingTab()) {
        area->UpdateTabDrag(InCursorPos);
    } else if (glm::length(InCursorPos - m_PressPosition) > 6.0f) {
        area->BeginTabDrag(m_Tabs[m_PressedTabIndex], this, InCursorPos);
    }
}

void UIDockNode::OnReleased(bool InInside) {
    (void)InInside;
    if (UIDockArea* area = GetArea()) {
        if (area->IsDraggingTab()) {
            area->EndTabDrag();
        }
    }
    m_PressedTabIndex = -1;
}

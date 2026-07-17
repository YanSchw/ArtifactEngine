#include "OutlinerTab.h"
#include "OutlinerRow.h"
#include "UI/EditorStyle.h"
#include "GameFramework/UIScrollArea.h"
#include "GameFramework/UIVStack.h"
#include "GameFramework/UIQuad.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UITextArea.h"
#include "GameFramework/UICanvas.h"
#include "Assets/AssetManager.h"
#include "Assets/VectorImage.h"
#include "Common/UUID.h"
#include "InputSystem/KeyboardDevice.h"
#include <algorithm>
#include <cctype>

OutlinerTab::OutlinerTab() {
    AssetManager& assets = AssetManager::Get();
    m_NodeIcon = assets.GetAsset<VectorImage>(UUID::FromString("b1c2d3e4-0001-4a00-9000-000000000001"));
    m_ArrowExpanded = assets.GetAsset<VectorImage>(UUID::FromString("b1c2d3e4-0002-4a00-9000-000000000002"));
    m_ArrowCollapsed = assets.GetAsset<VectorImage>(UUID::FromString("b1c2d3e4-0003-4a00-9000-000000000003"));
    m_EyeIcon = assets.GetAsset<VectorImage>(UUID::FromString("b1c2d3e4-0004-4a00-9000-000000000004"));
    m_EyeClosedIcon = assets.GetAsset<VectorImage>(UUID::FromString("b1c2d3e4-0005-4a00-9000-000000000005"));

    UIVStack* layout = Add<UIVStack>();
    layout->Fill();

    UIQuad* searchBar = layout->Add<UIQuad>();
    searchBar->Size = { 1.0_rel, 30.0f };
    searchBar->Color = EditorStyle::TabBar;

    m_SearchField = searchBar->Add<UITextArea>();
    m_SearchField->Center({ 1.0_rel - 12.0_px, 22.0f });
    m_SearchField->SingleLine = true;
    m_SearchField->Placeholder = "Search...";
    m_SearchField->FontSize = EditorStyle::FontSize;
    m_SearchField->TextColor = EditorStyle::Text;
    m_SearchField->PlaceholderColor = EditorStyle::TextDim;
    m_SearchField->BackgroundColor = EditorStyle::PanelDark;
    m_SearchField->FocusedBorderColor = EditorStyle::Accent;
    m_SearchField->Padding = UIPadding(6.0f, 0.0f);

    UIScrollArea* scroll = layout->Add<UIScrollArea>();
    scroll->Size = { 1.0_rel, 1.0_rel };  // whatever the fixed-height bars leave over
    scroll->Padding = UIPadding(4.0f, 4.0f);

    UIVStack* list = scroll->Add<UIVStack>();
    list->Anchor = list->Pivot = Vec2(0.0f);
    list->Position = Vec2(0.0f);
    list->Size = { 1.0_rel, 0.0_px };  // width fills; height grows with its rows
    m_List = list;

    list->Bind = [this] {
        RebuildVisible();

        int have = (int)m_List->GetChildCount();
        const int want = m_Visible.Size();
        while (have < want) {
            OutlinerRow* row = m_List->Add<OutlinerRow>();
            row->Owner = this;
            row->RowIndex = have;
            row->Size = { 1.0_rel, OutlinerRow::RowHeight };
            row->Interactable = true;
            have++;
        }
        while (have > want) {
            delete m_List->GetChild(have - 1);
            have--;
        }
        m_List->Size = { 1.0_rel, (float)want * OutlinerRow::RowHeight };

        RefreshFooter();
    };

    UIQuad* footer = layout->Add<UIQuad>();
    footer->Size = { 1.0_rel, 22.0f };
    footer->Color = EditorStyle::BottomBar;

    m_FooterLabel = footer->Add<UILabel>();
    m_FooterLabel->Anchor = m_FooterLabel->Pivot = Vec2(0.0f, 0.5f);
    m_FooterLabel->Position = Vec2(8.0f, 0.0f);
    m_FooterLabel->Size = { 1.0_rel - 16.0_px, 1.0_rel };
    m_FooterLabel->FontSize = EditorStyle::FontSize - 1.0f;
    m_FooterLabel->Color = EditorStyle::TextDim;
    m_FooterLabel->VAlign = UIVAlign::Middle;
}

void OutlinerTab::RefreshFooter() {
    World* world = GetEditedWorld();
    const int total = world ? world->GetAllNodes().Size() : 0;
    String text = HasFilter()
        ? std::to_string(m_MatchCount) + " of " + std::to_string(total) + " nodes"
        : std::to_string(total) + " nodes";
    if (m_Selected.Get()) {
        text += " (1 selected)";
    }
    m_FooterLabel->Text = text;
}

const OutlinerTab::VisibleRow* OutlinerTab::GetVisibleRow(int InIndex) const {
    if (InIndex < 0 || InIndex >= m_Visible.Size()) {
        return nullptr;
    }
    return &m_Visible[InIndex];
}

static String ToLower(const String& InText) {
    String lower = InText;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return lower;
}

static bool ContainsIgnoreCase(const String& InHaystack, const String& InLowerNeedle) {
    return ToLower(InHaystack).find(InLowerNeedle) != String::npos;
}

void OutlinerTab::RebuildVisible() {
    m_Visible.Clear();
    m_Filter = ToLower(m_SearchField->Text);
    m_MatchCount = 0;

    World* world = GetEditedWorld();
    if (!world) {
        return;
    }
    for (Node* node : world->GetAllNodes()) {
        if (node->GetParent()) {
            continue;
        }
        if (!HasFilter()) {
            AppendSubtree(node, 0);
        } else if (SubtreeMatches(node)) {
            AppendFilteredSubtree(node, 0);
        }
    }
}

void OutlinerTab::AppendSubtree(Node* InNode, int InDepth) {
    VisibleRow row;
    row.NodePtr = InNode;
    row.Depth = InDepth;
    row.HasChildren = InNode->HasChildren();
    m_Visible.Add(row);

    if (row.HasChildren && IsExpanded(InNode)) {
        for (uint32_t i = 0; i < InNode->GetChildCount(); i++) {
            AppendSubtree(InNode->GetChild((int)i), InDepth + 1);
        }
    }
}

bool OutlinerTab::MatchesFilter(Node* InNode) const {
    return ContainsIgnoreCase(InNode->GetName(), m_Filter)
        || ContainsIgnoreCase(InNode->GetClass().Name, m_Filter);
}

bool OutlinerTab::SubtreeMatches(Node* InNode) const {
    if (MatchesFilter(InNode)) {
        return true;
    }
    for (uint32_t i = 0; i < InNode->GetChildCount(); i++) {
        if (SubtreeMatches(InNode->GetChild((int)i))) {
            return true;
        }
    }
    return false;
}

// Appends a hit or an ancestor on a hit's path, fully expanded; other branches are pruned.
void OutlinerTab::AppendFilteredSubtree(Node* InNode, int InDepth) {
    VisibleRow row;
    row.NodePtr = InNode;
    row.Depth = InDepth;
    row.Matches = MatchesFilter(InNode);
    if (row.Matches) {
        m_MatchCount++;
    }

    Array<Node*> visibleChildren;
    for (uint32_t i = 0; i < InNode->GetChildCount(); i++) {
        Node* child = InNode->GetChild((int)i);
        if (SubtreeMatches(child)) {
            visibleChildren.Add(child);
        }
    }
    row.HasChildren = !visibleChildren.IsEmpty();
    m_Visible.Add(row);

    for (Node* child : visibleChildren) {
        AppendFilteredSubtree(child, InDepth + 1);
    }
}

void OutlinerTab::OnUIUpdate(const UIFrameContext& InContext) {
    (void)InContext;
    KeyboardDevice* keyboard = KeyboardDevice::Instance();
    if (!keyboard || !keyboard->IsDown(KeyCode::F2)) {
        return;
    }
    // F2 must not fire while typing in this (or any) text field.
    UICanvas* canvas = GetCanvas();
    UINode* focused = canvas ? canvas->GetFocusedNode() : nullptr;
    if (focused && focused->As<UITextArea>()) {
        return;
    }
    if (Node* selected = m_Selected.Get()) {
        if (m_Renaming.Get() != selected) {
            BeginRename(selected);
        }
    }
}

void OutlinerTab::ToggleExpanded(Node* InNode) {
    if (m_Collapsed.Contains(InNode)) {
        m_Collapsed.Remove(InNode);
    } else {
        m_Collapsed.Add(InNode);
    }
}

void OutlinerTab::BeginRename(Node* InNode) {
    m_Renaming = InNode;
}

void OutlinerTab::CommitRename(const String& InName) {
    if (Node* node = m_Renaming.Get()) {
        if (!InName.empty()) {
            node->SetName(InName);
        }
    }
    m_Renaming = nullptr;
}

void OutlinerTab::CancelRename() {
    m_Renaming = nullptr;
}

void OutlinerTab::DragOver(const Vec2& InScreenCursor) {
    m_DropRef = nullptr;
    m_DropMode = DropMode::None;
    for (uint32_t i = 0; i < m_List->GetChildCount(); i++) {
        OutlinerRow* row = m_List->GetChild((int)i)->As<OutlinerRow>();
        if (row && row->IsEnabled() && row->HitTest(InScreenCursor)) {
            m_DropRef = row->GetBoundNode();
            m_DropMode = row->HitZone(InScreenCursor);
            break;
        }
    }
}

void OutlinerTab::EndDrag() {
    Node* source = m_DragSource.Get();
    Node* ref = m_DropRef.Get();
    const DropMode mode = m_DropMode;

    m_DragSource = nullptr;
    m_DropRef = nullptr;
    m_DropMode = DropMode::None;

    if (!source || !ref || ref == source) {
        return;
    }

    if (mode == DropMode::Onto) {
        // ForceSetParent already rejects reparenting under a descendant.
        if (!ref->IsChildOf(source)) {
            source->SetParent(ref, false);
            m_Collapsed.Remove(ref);
        }
        return;
    }

    // Before/After: become a sibling of the reference node under its parent.
    Node* parent = ref->GetParent();
    if (!parent || parent == source || parent->IsChildOf(source)) {
        return;
    }
    source->SetParent(parent, false);  // detaches from old parent, appends under the new one
    const int32_t refIndex = ref->GetSiblingIndex();
    source->SetSiblingIndex(mode == DropMode::Before ? refIndex : refIndex + 1);
}

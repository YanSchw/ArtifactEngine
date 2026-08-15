#include "OutlinerTab.h"
#include "OutlinerRow.h"
#include "MajorTab.h"
#include "AnimationEditorTab.h"
#include "ThemedWindow.h"
#include "UI/EditorStyle.h"
#include "UI/EditorIcons.h"
#include "UI/EditorDragDrop.h"
#include "GameFramework/UIScrollArea.h"
#include "GameFramework/UIVStack.h"
#include "GameFramework/UIQuad.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UITextArea.h"
#include "GameFramework/UICanvas.h"
#include "Assets/AssetManager.h"
#include "Assets/NodeRecord.h"
#include "Assets/VectorImage.h"
#include "Common/UUID.h"
#include "InputSystem/Clipboard.h"
#include "InputSystem/KeyboardDevice.h"
#include <algorithm>
#include <cctype>

static const char* s_ClipboardKey = "ArtifactNodes";

VectorImage* OutlinerTab::GetTabIcon() const {
    return EditorIcons::Outliner();
}

bool OutlinerTab::IsAnimationRoot(Node* InNode) const {
    AnimationEditorTab* animation = GetMajorTab() ? GetMajorTab()->As<AnimationEditorTab>() : nullptr;
    return animation && animation->IsAnimationRoot(InNode);
}

void OutlinerTab::SetAnimationRoot(Node* InNode) {
    if (AnimationEditorTab* animation = GetMajorTab() ? GetMajorTab()->As<AnimationEditorTab>() : nullptr) {
        animation->SetAnimationRoot(InNode);
    }
}

static bool IsCommandHeld(KeyboardDevice& InKeyboard) {
    return InKeyboard.IsPressed(KeyCode::LeftControl) || InKeyboard.IsPressed(KeyCode::RightControl)
        || InKeyboard.IsPressed(KeyCode::LeftSuper) || InKeyboard.IsPressed(KeyCode::RightSuper);
}

static Array<SharedObjectPtr<NodeRecord>> ReadClipboardRecords() {
    Array<SharedObjectPtr<NodeRecord>> records;
    const nlohmann::json clipboard = nlohmann::json::parse(Clipboard::GetText(), nullptr, false);
    if (!clipboard.is_object() || !clipboard.contains(s_ClipboardKey) || !clipboard[s_ClipboardKey].is_array()) {
        return records;
    }
    for (const nlohmann::json& entry : clipboard[s_ClipboardKey]) {
        if (SharedObjectPtr<NodeRecord> record = NodeRecord::FromJson(entry)) {
            records.Add(record);
        }
    }
    return records;
}

OutlinerTab::OutlinerTab() {
    AssetManager& assets = AssetManager::Get();
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

    OutlinerRow* emptyArea = scroll->Add<OutlinerRow>();
    emptyArea->Owner = this;
    emptyArea->RowIndex = -1;
    emptyArea->Fill();
    emptyArea->Interactable = true;

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

    EditorDragDrop::AddDropZone(*this,
        [this](Asset* InAsset) {
            MajorTab* major = GetMajorTab();
            return major && major->GetAssetRootNode() && MajorTab::IsSpawnableAsset(InAsset);
        },
        [this](Asset* InAsset, const Vec2& InCursorPos) { SpawnDroppedAsset(InAsset, InCursorPos); });
}

void OutlinerTab::RefreshFooter() {
    World* world = GetEditedWorld();
    const int total = world ? world->GetAllNodes().Size() : 0;
    String text = HasFilter()
        ? std::to_string(m_MatchCount) + " of " + std::to_string(total) + " nodes"
        : std::to_string(total) + " nodes";
    const int32_t selected = GetMajorTab() ? GetMajorTab()->GetSelectionCount() : 0;
    if (selected > 0) {
        text += " (" + std::to_string(selected) + " selected)";
    }
    m_FooterLabel->Text = text;
}

bool OutlinerTab::IsSelected(Node* InNode) const {
    return GetMajorTab() && GetMajorTab()->IsSelected(InNode);
}

bool OutlinerTab::IsSoleSelected(Node* InNode) const {
    return GetMajorTab() && GetMajorTab()->GetSoleSelection() == InNode;
}

void OutlinerTab::HandleRowClick(Node* InNode, bool InToggle, bool InRange) {
    MajorTab* major = GetMajorTab();
    if (!major || !InNode) {
        return;
    }
    if (InRange) {
        int anchorIndex = -1;
        int nodeIndex = -1;
        for (int i = 0; i < m_Visible.Size(); i++) {
            if (m_Visible[i].NodePtr == m_SelectAnchor.Get()) {
                anchorIndex = i;
            }
            if (m_Visible[i].NodePtr == InNode) {
                nodeIndex = i;
            }
        }
        if (anchorIndex >= 0 && nodeIndex >= 0) {
            Array<Object*> range;
            for (int i = std::min(anchorIndex, nodeIndex); i <= std::max(anchorIndex, nodeIndex); i++) {
                range.Add(m_Visible[i].NodePtr);
            }
            major->SetSelection(range);
            return;
        }
    }
    if (InToggle) {
        major->ToggleSelection(InNode);
    } else {
        major->SetSelection(InNode);
    }
    m_SelectAnchor = InNode;
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

void OutlinerTab::SpawnDroppedAsset(Asset* InAsset, const Vec2& InCursorPos) {
    MajorTab* major = GetMajorTab();
    Node* root = major ? major->GetAssetRootNode() : nullptr;
    if (!root) {
        return;
    }

    // Dropping onto a row parents the new node under it; anywhere else lands at the scene root.
    Node* parent = root;
    for (uint32_t i = 0; i < m_List->GetChildCount(); i++) {
        OutlinerRow* row = m_List->GetChild((int)i)->As<OutlinerRow>();
        if (row && row->IsEnabled() && row->HitTest(InCursorPos) && row->GetBoundNode()) {
            parent = row->GetBoundNode();
            break;
        }
    }

    RecordEdit("Add Node", parent);
    Node* spawned = MajorTab::SpawnFromAsset(InAsset, *parent);
    if (!spawned) {
        return;
    }
    if (parent != root && !IsExpanded(parent)) {
        ToggleExpanded(parent);
    }
    major->SetSelection(spawned);
}

Array<Node*> OutlinerTab::GetCopyableSelection() const {
    Array<Node*> nodes;
    MajorTab* major = GetMajorTab();
    if (!major) {
        return nodes;
    }
    for (Object* selected : major->GetSelection()) {
        Node* node = Cast<Node>(selected);
        if (node && !major->IsAssetRootNode(node)) {
            nodes.Add(node);
        }
    }
    // A node a selected ancestor already brings along must not be copied a second time.
    for (int32_t i = nodes.Size() - 1; i >= 0; i--) {
        for (Node* other : nodes) {
            if (other != nodes[i] && nodes[i]->IsChildOf(other)) {
                nodes.RemoveAt(i);
                break;
            }
        }
    }
    return nodes;
}

void OutlinerTab::CopySelection() {
    const Array<Node*> nodes = GetCopyableSelection();
    if (nodes.IsEmpty()) {
        return;
    }
    nlohmann::json records = nlohmann::json::array();
    for (Node* node : nodes) {
        records.push_back(NodeRecord::Capture(*node)->ToJson());
    }
    nlohmann::json clipboard = nlohmann::json::object();
    clipboard[s_ClipboardKey] = records;
    Clipboard::SetText(clipboard.dump());
}

bool OutlinerTab::ClipboardHasNodes() {
    return !ReadClipboardRecords().IsEmpty();
}

Node* OutlinerTab::GetPasteParent() const {
    MajorTab* major = GetMajorTab();
    if (!major) {
        return nullptr;
    }
    Node* selected = Cast<Node>(major->GetSoleSelection());
    return (selected && selected->GetParent()) ? selected->GetParent() : major->GetAssetRootNode();
}

Node* OutlinerTab::PasteRecord(const NodeRecord& InRecord, Node& InParent) {
    Node* node = InRecord.Instantiate(&InParent);
    if (!node) {
        return nullptr;
    }
    if (!IsExpanded(&InParent)) {
        ToggleExpanded(&InParent);
    }
    return node;
}

void OutlinerTab::PasteInto(Node* InParent) {
    MajorTab* major = GetMajorTab();
    Node* parent = InParent ? InParent : GetPasteParent();
    const Array<SharedObjectPtr<NodeRecord>> records = ReadClipboardRecords();
    if (!major || !parent || records.IsEmpty()) {
        return;
    }

    RecordEdit("Paste", parent);
    Array<Object*> pasted;
    for (const SharedObjectPtr<NodeRecord>& record : records) {
        if (Node* node = PasteRecord(*record, *parent)) {
            pasted.Add(node);
        }
    }
    if (!pasted.IsEmpty()) {
        major->SetSelection(pasted);
    }
}

void OutlinerTab::DuplicateSelection() {
    MajorTab* major = GetMajorTab();
    const Array<Node*> nodes = GetCopyableSelection();
    if (!major || nodes.IsEmpty()) {
        return;
    }

    Array<SharedObjectPtr<NodeRecord>> records;
    Array<Node*> parents;
    for (Node* node : nodes) {
        if (Node* parent = node->GetParent()) {
            records.Add(NodeRecord::Capture(*node));
            parents.Add(parent);
        }
    }
    for (Node* parent : parents) {
        RecordEdit("Duplicate", parent);
    }

    Array<Object*> duplicates;
    for (int32_t i = 0; i < records.Size(); i++) {
        if (Node* node = PasteRecord(*records[i], *parents[i])) {
            duplicates.Add(node);
        }
    }
    if (!duplicates.IsEmpty()) {
        major->SetSelection(duplicates);
    }
}

void OutlinerTab::OnUIUpdate(const UIFrameContext& InContext) {
    (void)InContext;
    KeyboardDevice* keyboard = KeyboardDevice::Instance();
    if (!keyboard || !AcceptsShortcuts()) {
        return;
    }

    if (IsCommandHeld(*keyboard)) {
        if (keyboard->IsDown(KeyCode::C)) {
            CopySelection();
        } else if (keyboard->IsDown(KeyCode::V)) {
            PasteInto(nullptr);
        } else if (keyboard->IsDown(KeyCode::D)) {
            DuplicateSelection();
        }
        return;
    }

    if (keyboard->IsDown(KeyCode::F2)) {
        Node* selected = GetMajorTab() ? Cast<Node>(GetMajorTab()->GetSoleSelection()) : nullptr;
        if (selected && m_Renaming.Get() != selected) {
            BeginRename(selected);
        }
    }
}

bool OutlinerTab::IsExpanded(Node* InNode) const {
    for (const WeakObjectPtr<Node>& collapsed : m_Collapsed) {
        if (collapsed.Get() == InNode) {
            return false;
        }
    }
    return true;
}

void OutlinerTab::ToggleExpanded(Node* InNode) {
    const bool collapse = IsExpanded(InNode);
    // Entries whose node died drop out on the way past.
    for (int32_t i = m_Collapsed.Size() - 1; i >= 0; i--) {
        Node* collapsed = m_Collapsed[i].Get();
        if (!collapsed || collapsed == InNode) {
            m_Collapsed.RemoveAt(i);
        }
    }
    if (collapse) {
        m_Collapsed.Add(InNode);
    }
}

void OutlinerTab::BeginRename(Node* InNode) {
    MajorTab* major = GetMajorTab();
    if (major && major->IsAssetRootNode(InNode)) {
        return;
    }
    m_Renaming = InNode;
}

void OutlinerTab::CommitRename(const String& InName) {
    if (Node* node = m_Renaming.Get()) {
        if (!InName.empty()) {
            RecordEdit("Rename", node);
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

    if (!source || !ref || ref == source || source->IsInherited()) {
        return;
    }

    RecordEdit("Move Node", source->GetRootNode());

    if (mode == DropMode::Onto) {
        // ForceSetParent already rejects reparenting under a descendant.
        if (!ref->IsChildOf(source)) {
            source->SetParent(ref, false);
            if (!IsExpanded(ref)) {
                ToggleExpanded(ref);
            }
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

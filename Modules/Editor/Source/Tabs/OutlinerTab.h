#pragma once
#include "MinorTab.h"
#include "Object/Pointer.h"
#include "Common/Array.h"
#include "OutlinerTab.gen.h"

class Node;
class VectorImage;
class UITextArea;
class UILabel;

/** Lists the nodes of the edited scene. */
class OutlinerTab : public MinorTab {
public:
    ARTIFACT_CLASS();

    OutlinerTab();

    virtual String GetTabTitle() const override { return "Outliner"; }

    /** Where a dragged row lands: onto a node (become its child) or between rows (become a sibling
     *  just above/below the reference node). */
    enum class DropMode : uint8_t { None, Onto, Before, After };

    /** One entry of the flattened, currently-visible tree; the row widget at the same index binds
     *  to it each frame. */
    struct VisibleRow {
        Node* NodePtr = nullptr;
        int Depth = 0;
        bool HasChildren = false;
        /* False for a row only kept to show a search hit's path from the root. */
        bool Matches = true;
    };

    const VisibleRow* GetVisibleRow(int InIndex) const;
    int GetVisibleCount() const { return m_Visible.Size(); }

    bool IsSelected(Node* InNode) const;
    bool IsSoleSelected(Node* InNode) const;
    void HandleRowClick(Node* InNode, bool InToggle, bool InRange);

    bool IsExpanded(Node* InNode) const { return !m_Collapsed.Contains(InNode); }
    void ToggleExpanded(Node* InNode);

    /** A search filter is active; the tree shows matches (and their ancestors) fully expanded. */
    bool HasFilter() const { return !m_Filter.empty(); }

    Node* GetRenamingNode() const { return m_Renaming.Get(); }
    void BeginRename(Node* InNode);
    void CommitRename(const String& InName);
    void CancelRename();

    void BeginDrag(Node* InNode) { m_DragSource = InNode; m_DropRef = nullptr; m_DropMode = DropMode::None; }
    void DragOver(const Vec2& InScreenCursor);
    void EndDrag();
    Node* GetDragSource() const { return m_DragSource.Get(); }
    Node* GetDropRef() const { return m_DropRef.Get(); }
    DropMode GetDropMode() const { return m_DropMode; }

    VectorImage* GetArrowIcon(bool InExpanded) const { return InExpanded ? m_ArrowExpanded : m_ArrowCollapsed; }
    VectorImage* GetEyeIcon(bool InOpen) const { return InOpen ? m_EyeIcon : m_EyeClosedIcon; }

    virtual void OnUIUpdate(const UIFrameContext& InContext) override;

private:
    void RebuildVisible();
    void AppendSubtree(Node* InNode, int InDepth);
    bool MatchesFilter(Node* InNode) const;
    bool SubtreeMatches(Node* InNode) const;
    void AppendFilteredSubtree(Node* InNode, int InDepth);
    void RefreshFooter();

    UINode* m_List = nullptr;
    UITextArea* m_SearchField = nullptr;
    UILabel* m_FooterLabel = nullptr;

    Array<VisibleRow> m_Visible;
    Array<Node*> m_Collapsed;
    String m_Filter;  // lowercased search text, rebuilt each frame
    int m_MatchCount = 0;

    WeakObjectPtr<Node> m_SelectAnchor;
    WeakObjectPtr<Node> m_Renaming;
    WeakObjectPtr<Node> m_DragSource;
    WeakObjectPtr<Node> m_DropRef;
    DropMode m_DropMode = DropMode::None;

    VectorImage* m_ArrowExpanded = nullptr;
    VectorImage* m_ArrowCollapsed = nullptr;
    VectorImage* m_EyeIcon = nullptr;
    VectorImage* m_EyeClosedIcon = nullptr;
};

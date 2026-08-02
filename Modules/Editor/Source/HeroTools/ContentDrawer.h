#pragma once
#include "HeroTool.h"
#include "Common/String.h"
#include "Common/Array.h"
#include "Common/Types.h"
#include "Object/Pointer.h"
#include <functional>
#include "ContentDrawer.gen.h"

class UINode;
class UIStack;
class UITextArea;
class ThumbnailRenderer;
class Asset;
class UIMenuModel;

class ContentDrawer : public HeroTool {
public:
    ARTIFACT_CLASS();

    virtual String GetTitle() const override { return "Content Drawer"; }
    virtual void BuildDrawer(UINode& InBody) override;
    virtual void Tick(float InDeltaTime) override;

private:
    struct Location {
        String Mount;
        String Rel;
    };

    struct Item {
        bool IsFolder = false;
        String Name;
        String Path;
        String Rel;
        Class AssetClass;
        Asset* AssetPtr = nullptr;
    };

    struct InlineEdit {
        bool Active = false;
        bool IsFolder = false;
        String Path;
        String Name;
        Class AssetClass;
        std::function<void(const String&)> Commit;
    };

    struct DropTarget {
        WeakObjectPtr<UINode> Node;
        String Mount;
        String Rel;
    };

    void NavigateTo(const String& InMount, const String& InRel, bool InPushHistory = true);
    void GoBack();
    void GoForward();

    void RebuildContent();
    void BuildBreadcrumb();
    void BuildTree();
    void BuildTreeNode(const String& InMount, const String& InRel, const String& InName, int32_t InDepth);
    void BuildGrid();
    Array<Item> CollectItems() const;
    void BuildFolderTile(const Item& InItem);
    void BuildAssetTile(const Item& InItem);
    UITextArea* AddEditField(UINode& InParent);
    bool IsEditing(const Item& InItem) const { return m_Edit.Active && m_Edit.Path == InItem.Path; }

    void BuildAddMenu(UIMenuModel& OutMenu);
    void BuildItemMenu(UIMenuModel& OutMenu, const Item& InItem);
    void BuildFolderMenu(UIMenuModel& OutMenu, const String& InMount, const String& InRel);

    void BeginCreateFolder();
    void BeginCreateAsset(const Class& InAssetClass, const String& InBaseName,
                          std::function<Asset*(const String&, const String&)> InFactory);
    void BeginRename(const Item& InItem);
    void CommitEdit(const String& InName);
    void CancelEdit();
    void OpenNewBlueprintDialog();

    bool RenameItem(const Item& InItem, const String& InNewName);
    bool MoveItem(const Item& InItem, const String& InMount, const String& InRel);
    bool DeleteItem(const Item& InItem);
    void PruneLocations();

    void BeginDrag(const Item& InItem);
    void DragOver(const Vec2& InCursorPos);
    void EndDrag();
    void RegisterDropTarget(UINode& InNode, const String& InMount, const String& InRel);
    bool IsDropHighlight(const UINode& InNode) const { return m_Dragging && m_DropNode.Get() == &InNode; }

    String MakeUniqueName(const String& InDir, const String& InBaseName, bool InFolder) const;
    void OpenAsset(Asset* InAsset);

    bool IsExpanded(const String& InKey) const;
    void SetExpanded(const String& InKey, bool InExpanded);
    String DirFor(const String& InMount, const String& InRel) const;
    bool IsCurrent(const String& InMount, const String& InRel) const;
    bool IsMountRoot(const String& InRel) const { return InRel.empty(); }

    String m_Mount;
    String m_RelPath;
    String m_SelectedPath;
    Array<String> m_Expanded;
    Array<Location> m_History;
    int32_t m_HistoryPos = -1;
    bool m_NavDirty = false;

    InlineEdit m_Edit;

    Array<DropTarget> m_DropTargets;
    Item m_DragItem;
    bool m_Dragging = false;
    WeakObjectPtr<UINode> m_DropNode;
    Location m_DropLocation;

    UIStack* m_Breadcrumb = nullptr;
    UIStack* m_Tree = nullptr;
    UINode* m_Grid = nullptr;
    SharedObjectPtr<ThumbnailRenderer> m_Thumbnails;
};

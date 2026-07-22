#pragma once
#include "HeroTool.h"
#include "Common/String.h"
#include "Common/Array.h"
#include "Object/Pointer.h"
#include "ContentDrawer.gen.h"

class UINode;
class UIStack;
class ThumbnailRenderer;

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

    void NavigateTo(const String& InMount, const String& InRel, bool InPushHistory = true);
    void GoBack();
    void GoForward();

    void RebuildContent();
    void BuildBreadcrumb();
    void BuildTree();
    void BuildTreeNode(const String& InMount, const String& InRel, const String& InName, int32_t InDepth);
    void BuildGrid();

    bool IsExpanded(const String& InKey) const;
    void SetExpanded(const String& InKey, bool InExpanded);
    String DirFor(const String& InMount, const String& InRel) const;
    bool IsCurrent(const String& InMount, const String& InRel) const;

    String m_Mount;
    String m_RelPath;
    String m_SelectedPath;
    Array<String> m_Expanded;
    Array<Location> m_History;
    int32_t m_HistoryPos = -1;
    bool m_NavDirty = false;

    UIStack* m_Breadcrumb = nullptr;
    UIStack* m_Tree = nullptr;
    UINode* m_Grid = nullptr;
    SharedObjectPtr<ThumbnailRenderer> m_Thumbnails;
};

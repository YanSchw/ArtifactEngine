#pragma once
#include "MinorTab.h"
#include "Object/Pointer.h"
#include "DetailsTab.gen.h"

class UIQuad;
class UITextArea;
class UILabel;
class UIScrollArea;
class UIVStack;
class DetailsCustomization;
class VectorImage;

/** Edits the selected object of the MajorTab through its DetailsCustomization. */
class DetailsTab : public MinorTab {
public:
    ARTIFACT_CLASS();

    DetailsTab();

    virtual String GetTabTitle() const override { return "Details"; }
    virtual VectorImage* GetTabIcon() const override;

    float GetLabelSplit() const { return m_LabelSplit; }
    void SetLabelSplit(float InSplit);

    const String& GetFilter() const { return m_Filter; }
    static bool MatchesFilter(const String& InText, const String& InLowerFilter);

    class ThumbnailRenderer& GetThumbnails();

    void MarkDirty() { m_RebuildPending = true; }

    bool IsCollapsed(const String& InKey) const;
    void SetCollapsed(const String& InKey, bool InCollapsed);

    /** Decides whether a category the user has not toggled yet starts collapsed. */
    std::function<bool(const String& InKey)> CollapsedByDefault;

    /** Inspects this object instead of the MajorTab's selection, for panels outside of one. */
    void Inspect(Object* InObject) { m_Pinned = InObject; }

private:
    void Refresh();
    void RebuildInspector(Object* InObject, DetailsCustomization* InCustomization);

    UINode* m_HeaderHost = nullptr;
    UIQuad* m_SearchBar = nullptr;
    UITextArea* m_SearchField = nullptr;
    UIScrollArea* m_Scroll = nullptr;
    UIVStack* m_List = nullptr;
    UILabel* m_MessageLabel = nullptr;

    WeakObjectPtr<Object> m_Inspected;
    WeakObjectPtr<Object> m_Pinned;
    SharedObjectPtr<class ThumbnailRenderer> m_Thumbnails;
    float m_HeaderHeight = 0.0f;
    float m_LabelSplit = 0.32f;
    String m_Filter;
    Array<String> m_CollapsedKeys;
    Array<String> m_ExpandedKeys;
    bool m_RebuildPending = false;
    uint64_t m_PropertyRegistryVersion = 0;
};

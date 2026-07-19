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

/** Edits the selected object of the MajorTab through its DetailsCustomization. */
class DetailsTab : public MinorTab {
public:
    ARTIFACT_CLASS();

    DetailsTab();

    virtual String GetTabTitle() const override { return "Details"; }

    float GetLabelSplit() const { return m_LabelSplit; }
    void SetLabelSplit(float InSplit);

    const String& GetFilter() const { return m_Filter; }
    static bool MatchesFilter(const String& InText, const String& InLowerFilter);

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
    float m_HeaderHeight = 0.0f;
    float m_LabelSplit = 0.32f;
    String m_Filter;
};

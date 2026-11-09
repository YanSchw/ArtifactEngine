#pragma once
#include "GameFramework/UINode.h"
#include "Object/Object.h"
#include "Common/Map.h"
#include <functional>
#include "UIClassPicker.gen.h"

class UIStack;
class UITextArea;

/** Picks a class out of the hierarchy below RootClass. */
class UIClassPicker : public UINode {
public:
    ARTIFACT_CLASS();

    UIClassPicker();

    Class RootClass = Class("Node");
    Class Selected;
    /** Classes failing this are left out, along with everything below them. */
    std::function<bool(const Class&)> Filter;
    std::function<void(const Class&)> Confirmed;

    static bool IsPlaceableNodeClass(const Class& InClass);

    void FocusSearch();

    virtual void OnBind() override;

private:
    void CollectHierarchy();
    void Rebuild();
    bool IsVisible(const Class& InClass) const;
    void AddRow(const Class& InClass, int32_t InDepth);
    void OnRowClicked(const Class& InClass);

    UITextArea* m_Search = nullptr;
    UIStack* m_Rows = nullptr;
    Map<String, Array<Class>> m_Children;
    Map<String, bool> m_Pickable;
    Array<String> m_Expanded;
    String m_Filter;
    bool m_HierarchyBuilt = false;
    bool m_Dirty = true;
    Class m_LastClicked;
    double m_LastClickTime = 0.0;
};

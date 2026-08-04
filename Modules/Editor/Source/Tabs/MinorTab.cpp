#include "MinorTab.h"
#include "MajorTab.h"
#include "UI/UIDockNode.h"
#include "UI/EditorIcons.h"

MinorTab::MinorTab() {
    Fill();
}

World* MinorTab::GetEditedWorld() const {
    if (MajorTab* major = m_MajorTab.Get()) {
        return major->GetEditedWorld();
    }
    return m_EditedWorld.Get();
}

VectorImage* MinorTab::GetTabIcon() const {
    return EditorIcons::Document();
}

UIDockNode* MinorTab::GetDockNode() const {
    return GetParent() ? GetParent()->As<UIDockNode>() : nullptr;
}

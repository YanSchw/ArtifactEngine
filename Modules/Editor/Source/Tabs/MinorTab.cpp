#include "MinorTab.h"
#include "UI/UIDockNode.h"
#include "UI/EditorIcons.h"

MinorTab::MinorTab() {
    Fill();
}

VectorImage* MinorTab::GetTabIcon() const {
    return EditorIcons::Document();
}

UIDockNode* MinorTab::GetDockNode() const {
    return GetParent() ? GetParent()->As<UIDockNode>() : nullptr;
}

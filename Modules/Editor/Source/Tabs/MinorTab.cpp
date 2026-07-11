#include "MinorTab.h"
#include "UI/UIDockNode.h"

MinorTab::MinorTab() {
    Fill();
}

UIDockNode* MinorTab::GetDockNode() const {
    return GetParent() ? GetParent()->As<UIDockNode>() : nullptr;
}

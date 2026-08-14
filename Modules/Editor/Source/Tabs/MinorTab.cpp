#include "MinorTab.h"
#include "MajorTab.h"
#include "ThemedWindow.h"
#include "UI/UIDockNode.h"
#include "UI/EditorIcons.h"
#include "GameFramework/UICanvas.h"
#include "GameFramework/UITextArea.h"

MinorTab::MinorTab() {
    Fill();
}

World* MinorTab::GetEditedWorld() const {
    if (MajorTab* major = m_MajorTab.Get()) {
        return major->GetEditedWorld();
    }
    return m_EditedWorld.Get();
}

void MinorTab::RecordEdit(const String& InTitle, Object* InObject) {
    if (MajorTab* major = GetMajorTab()) {
        major->BeginTransaction(InTitle, InObject);
    }
}

bool MinorTab::AcceptsShortcuts() const {
    UICanvas* canvas = GetCanvas();
    if (!canvas) {
        return false;
    }
    // Shortcuts must not fire while typing in this (or any) text field.
    UINode* focused = canvas->GetFocusedNode();
    if (focused && focused->As<UITextArea>()) {
        return false;
    }
    for (const SharedObjectPtr<ThemedWindow>& window : ThemedWindow::GetAllWindows()) {
        if (window.Get() && window->GetCanvas() == canvas) {
            return window->IsFocused();
        }
    }
    return false;
}

VectorImage* MinorTab::GetTabIcon() const {
    return EditorIcons::Document();
}

UIDockNode* MinorTab::GetDockNode() const {
    return GetParent() ? GetParent()->As<UIDockNode>() : nullptr;
}

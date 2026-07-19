#include "MajorTab.h"
#include "MinorTab.h"
#include "MinorTabStandaloneWindow.h"
#include "UI/UIDockArea.h"

static void AssignWorldToTabs(UIDockNode* InNode, World* InWorld) {
    if (!InNode) {
        return;
    }
    for (MinorTab* tab : InNode->GetTabs()) {
        tab->SetEditedWorld(InWorld);
    }
    AssignWorldToTabs(InNode->GetChildA(), InWorld);
    AssignWorldToTabs(InNode->GetChildB(), InWorld);
}

void MajorTab::SetEditedWorld(World* InWorld) {
    m_World = InWorld;
    AssignWorldToTabs(m_DockArea->GetRoot(), InWorld);
    for (WeakObjectPtr<MinorTabStandaloneWindow>& weak : m_FloatingWindows) {
        if (MinorTabStandaloneWindow* window = weak.Get()) {
            if (MinorTab* tab = window->GetTab()) {
                tab->SetEditedWorld(InWorld);
            }
        }
    }
}

MajorTab::MajorTab() {
    Fill();
    m_DockArea = Add<UIDockArea>();
}

Array<Object*> MajorTab::GetSelection() const {
    Array<Object*> alive;
    for (const WeakObjectPtr<Object>& weak : m_Selection) {
        if (Object* object = weak.Get()) {
            alive.Add(object);
        }
    }
    return alive;
}

int32_t MajorTab::GetSelectionCount() const {
    return GetSelection().Size();
}

Object* MajorTab::GetSoleSelection() const {
    Array<Object*> alive = GetSelection();
    return alive.Size() == 1 ? alive[0] : nullptr;
}

bool MajorTab::IsSelected(Object* InObject) const {
    if (!InObject) {
        return false;
    }
    for (const WeakObjectPtr<Object>& weak : m_Selection) {
        if (weak.Get() == InObject) {
            return true;
        }
    }
    return false;
}

void MajorTab::SetSelection(Object* InObject) {
    m_Selection.Clear();
    AddToSelection(InObject);
}

void MajorTab::SetSelection(const Array<Object*>& InObjects) {
    m_Selection.Clear();
    for (Object* object : InObjects) {
        AddToSelection(object);
    }
}

void MajorTab::AddToSelection(Object* InObject) {
    if (InObject && !IsSelected(InObject)) {
        m_Selection.Add(WeakObjectPtr<Object>(InObject));
    }
}

void MajorTab::RemoveFromSelection(Object* InObject) {
    for (int32_t i = 0; i < m_Selection.Size(); i++) {
        if (m_Selection[i].Get() == InObject) {
            m_Selection.RemoveAt(i);
            return;
        }
    }
}

void MajorTab::ToggleSelection(Object* InObject) {
    if (IsSelected(InObject)) {
        RemoveFromSelection(InObject);
    } else {
        AddToSelection(InObject);
    }
}

void MajorTab::ClearSelection() {
    m_Selection.Clear();
}

MajorTab::~MajorTab() {
    for (WeakObjectPtr<MinorTabStandaloneWindow>& weak : m_FloatingWindows) {
        if (MinorTabStandaloneWindow* window = weak.Get()) {
            ThemedWindow::DestroyWindow(window);
        }
    }
}

void MajorTab::FloatTab(MinorTab* InTab, const Vec2& InScreenPos) {
    SharedObjectPtr<MinorTabStandaloneWindow> window = MinorTabStandaloneWindow::Create(InTab, this, InScreenPos);
    m_FloatingWindows.Add(WeakObjectPtr<MinorTabStandaloneWindow>(window.Get()));
}

void MajorTab::ReDockFloatingTab(MinorTabStandaloneWindow* InWindow) {
    for (int32_t i = 0; i < m_FloatingWindows.Size(); i++) {
        if (m_FloatingWindows[i].Get() == InWindow) {
            m_FloatingWindows.RemoveAt(i);
            break;
        }
    }
    if (MinorTab* tab = InWindow->GetTab()) {
        m_DockArea->Dock(tab, UIDockSlot::Center);
    }
}

void MajorTab::SetFloatingWindowsVisible(bool InVisible) {
    for (WeakObjectPtr<MinorTabStandaloneWindow>& weak : m_FloatingWindows) {
        if (MinorTabStandaloneWindow* window = weak.Get()) {
            if (InVisible) {
                window->Show();
            } else {
                window->Hide();
            }
        }
    }
}

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

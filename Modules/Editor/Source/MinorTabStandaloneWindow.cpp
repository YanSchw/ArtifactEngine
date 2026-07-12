#include "MinorTabStandaloneWindow.h"
#include "Tabs/MinorTab.h"
#include "Tabs/MajorTab.h"
#include "UI/EditorStyle.h"
#include "GameFramework/UIQuad.h"

MinorTabStandaloneWindow::MinorTabStandaloneWindow(const WindowParams& InParams)
    : ThemedWindow(InParams) {
}

SharedObjectPtr<MinorTabStandaloneWindow> MinorTabStandaloneWindow::Create(MinorTab* InTab, MajorTab* InOwner, const Vec2& InScreenPos) {
    WindowParams params;
    params.Title = InTab->GetTabTitle();
    params.Width = 480;
    params.Height = 360;
    params.EditorStyle = true;

    SharedObjectPtr<MinorTabStandaloneWindow> window(new MinorTabStandaloneWindow(params));
    window->m_Tab = InTab;
    window->m_Owner = InOwner;
    window->SetPosition(InScreenPos - Vec2(80.0f, 14.0f));

    UIQuad* background = window->GetContentRoot()->Add<UIQuad>();
    background->Fill();
    background->Color = EditorStyle::Panel;
    InTab->SetParent(background);

    ThemedWindow::RegisterWindow(window);
    return window;
}

bool MinorTabStandaloneWindow::OnCloseRequested() {
    if (m_Owner && m_Tab) {
        m_Owner->ReDockFloatingTab(this);
    }
    return true;
}

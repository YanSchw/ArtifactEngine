#include "NewAnimationWindow.h"

#include "UI/EditorStyle.h"
#include "UI/EditorIcons.h"
#include "UI/UIClassPicker.h"
#include "Assets/Animation.h"
#include "Assets/Asset.h"
#include "GameFramework/Node3D.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UITextArea.h"
#include "Core/Log.h"
#include <filesystem>

NewAnimationWindow::NewAnimationWindow(const WindowParams& InParams)
    : DialogWindow(InParams) {
}

NewAnimationWindow* NewAnimationWindow::Open(const String& InDirectory, const String& InName, CreatedFn InCreated) {
    Window* opener = CurrentOpener();

    SharedObjectPtr<NewAnimationWindow> window(new NewAnimationWindow(MakeParams("New Animation", 420, 400)));
    window->m_Directory = InDirectory;
    window->m_Name = InName;
    window->m_PlaceholderClass = Node3D::StaticClass();
    window->m_Created = std::move(InCreated);
    window->BuildContent();

    Present(window, opener);
    return window.Get();
}

void NewAnimationWindow::BuildContent() {
    BuildFrame();

    AddCaption("Placeholder Root");

    m_ClassPicker = AddFillPanel()->Add<UIClassPicker>();
    m_ClassPicker->Fill();
    m_ClassPicker->Selected = m_PlaceholderClass;
    m_ClassPicker->Filter = &UIClassPicker::IsPlaceableNodeClass;
    m_ClassPicker->Confirmed = [this](const Class&) { Create(); };

    UITextArea* nameField = AddNamedTextField("Name");
    nameField->Text = m_Name;
    nameField->TextChanged = [this](const String& InText) { m_Name = InText; };
    nameField->Submitted = [this](const String&) { Create(); };
    nameField->SelectAll();
    nameField->RequestFocus();

    AddFooterButton("Cancel", false, [this] { Close(); });
    AddFooterButton("Create", true, [this] { Create(); });
}

void NewAnimationWindow::Create() {
    m_PlaceholderClass = m_ClassPicker->Selected;
    const String name = SanitizeName(m_Name);
    if (name.empty()) {
        AE_WARN("An Animation needs a name");
        return;
    }
    const String path = m_Directory + "/" + name + ".asset";
    if (std::filesystem::exists(path)) {
        AE_WARN("'{0}' already exists", path);
        return;
    }

    Animation* animation = Animation::CreateEmpty(m_Directory, name, m_PlaceholderClass);
    if (!animation) {
        return;
    }

    if (m_Created) {
        m_Created(animation);
    }
    Close();
}
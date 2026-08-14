#include "NewAnimationWindow.h"

#include "UI/EditorStyle.h"
#include "UI/EditorIcons.h"
#include "Assets/Animation.h"
#include "Assets/Asset.h"
#include "GameFramework/Component.h"
#include "GameFramework/Node3D.h"
#include "GameFramework/UIButton.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UIScrollArea.h"
#include "GameFramework/UITextArea.h"
#include "GameFramework/UIVStack.h"
#include "Core/Log.h"
#include <cctype>
#include <filesystem>

static constexpr float s_ClassRowHeight = 22.0f;

static String Lowered(const String& InText) {
    String lowered = InText;
    for (char& c : lowered) {
        c = (char)std::tolower((unsigned char)c);
    }
    return lowered;
}

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

    UITextArea* search = AddTextField("Search classes");
    search->TextChanged = [this](const String& InText) {
        m_Filter = InText;
        m_ListDirty = true;
    };

    UIScrollArea* scroll = AddFillPanel()->Add<UIScrollArea>();
    scroll->Fill();
    m_ClassList = scroll->Add<UIVStack>();
    m_ClassList->Fill();
    m_ClassList->Padding = UIPadding(2.0f, 2.0f);

    UITextArea* nameField = AddNamedTextField("Name");
    nameField->Text = m_Name;
    nameField->TextChanged = [this](const String& InText) { m_Name = InText; };
    nameField->Submitted = [this](const String&) { Create(); };
    nameField->SelectAll();
    nameField->RequestFocus();

    AddFooterButton("Cancel", false, [this] { Close(); });
    AddFooterButton("Create", true, [this] { Create(); });
}

void NewAnimationWindow::BuildClassList() {
    m_ListDirty = false;
    while (m_ClassList->HasChildren()) {
        delete m_ClassList->GetChild(0);
    }

    const String filter = Lowered(m_Filter);
    Array<Class> classes = Class::GetSubclassesOf(Node::StaticClass());
    classes.Sort([](const Class& InA, const Class& InB) { return InA.Name < InB.Name; });

    for (const Class& nodeClass : classes) {
        if (nodeClass.IsSubclassOf(Component::StaticClass())) {
            continue;
        }
        if (!filter.empty() && Lowered(nodeClass.Name).find(filter) == String::npos) {
            continue;
        }
        // Reflected classes without a default constructor cannot be spawned, and probing is the
        // only way to tell them apart.
        Object* probe = Object::Create(nodeClass);
        if (!probe) {
            continue;
        }
        delete probe;

        UIButton& row = EditorStyle::IconButton(*m_ClassList, EditorIcons::GetNodeIcon(nodeClass), EditorStyle::Text,
                                                nodeClass.Name, 0.0f,
                                                [this, nodeClass] { m_PlaceholderClass = nodeClass; });
        row.Size = { 1.0_rel, UIValue(s_ClassRowHeight) };
        row.Bind = [this, button = &row, nodeClass] {
            button->NormalColor = m_PlaceholderClass == nodeClass ? EditorStyle::Accent : Vec4(0.0f);
        };
    }
}

void NewAnimationWindow::Create() {
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

void NewAnimationWindow::PreUIRender(double InDeltaTime) {
    DialogWindow::PreUIRender(InDeltaTime);
    if (m_ListDirty) {
        BuildClassList();
    }
}

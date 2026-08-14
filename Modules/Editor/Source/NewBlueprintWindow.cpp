#include "NewBlueprintWindow.h"

#include "UI/EditorStyle.h"
#include "UI/EditorIcons.h"
#include "Assets/Asset.h"
#include "Assets/AssetManager.h"
#include "Assets/Blueprint.h"
#include "Assets/NodeRecord.h"
#include "GameFramework/Component.h"
#include "GameFramework/Node3D.h"
#include "GameFramework/UIButton.h"
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

NewBlueprintWindow::NewBlueprintWindow(const WindowParams& InParams)
    : DialogWindow(InParams) {
}

NewBlueprintWindow* NewBlueprintWindow::Open(const String& InDirectory, const String& InName, CreatedFn InCreated) {
    Window* opener = CurrentOpener();

    SharedObjectPtr<NewBlueprintWindow> window(new NewBlueprintWindow(MakeParams("New Blueprint", 420, 390)));
    window->m_Directory = InDirectory;
    window->m_Name = InName;
    window->m_ParentClass = Node3D::StaticClass();
    window->m_Created = std::move(InCreated);
    window->BuildContent();

    Present(window, opener);
    return window.Get();
}

void NewBlueprintWindow::BuildContent() {
    BuildFrame();

    AddCaption("Parent Class");

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

void NewBlueprintWindow::BuildClassList() {
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
                                                [this, nodeClass] { m_ParentClass = nodeClass; });
        row.Size = { 1.0_rel, UIValue(s_ClassRowHeight) };
        row.Bind = [this, button = &row, nodeClass] {
            button->NormalColor = m_ParentClass == nodeClass ? EditorStyle::Accent : Vec4(0.0f);
        };
    }
}

void NewBlueprintWindow::Create() {
    const String name = SanitizeName(m_Name);
    if (name.empty()) {
        AE_WARN("A Blueprint needs a name");
        return;
    }
    const String path = m_Directory + "/" + name + ".asset";
    if (std::filesystem::exists(path)) {
        AE_WARN("'{0}' already exists", path);
        return;
    }

    Blueprint* blueprint = Cast<Blueprint>(AssetManager::Get().CreateAsset(Blueprint::StaticClass(), m_Directory, name));
    if (!blueprint) {
        return;
    }
    NodeRecord* record = new NodeRecord();
    record->ClassName = m_ParentClass.Name;
    blueprint->SetRoot(SharedObjectPtr<NodeRecord>(record));
    AssetManager::Get().SaveAsset(blueprint);

    if (m_Created) {
        m_Created(blueprint);
    }
    Close();
}

void NewBlueprintWindow::PreUIRender(double InDeltaTime) {
    DialogWindow::PreUIRender(InDeltaTime);
    if (m_ListDirty) {
        BuildClassList();
    }
}

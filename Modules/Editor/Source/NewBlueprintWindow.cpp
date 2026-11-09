#include "NewBlueprintWindow.h"

#include "UI/EditorStyle.h"
#include "UI/EditorIcons.h"
#include "UI/UIClassPicker.h"
#include "Assets/Asset.h"
#include "Assets/AssetManager.h"
#include "Assets/Blueprint.h"
#include "Assets/NodeRecord.h"
#include "GameFramework/Node3D.h"
#include "GameFramework/UITextArea.h"
#include "Core/Log.h"
#include <filesystem>

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

    m_ClassPicker = AddFillPanel()->Add<UIClassPicker>();
    m_ClassPicker->Fill();
    m_ClassPicker->Selected = m_ParentClass;
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

void NewBlueprintWindow::Create() {
    m_ParentClass = m_ClassPicker->Selected;
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

#include "NewDataAssetWindow.h"

#include "UI/UIClassPicker.h"
#include "Assets/AssetManager.h"
#include "Assets/DataAsset.h"
#include "GameFramework/UITextArea.h"
#include "Core/Log.h"
#include <filesystem>

NewDataAssetWindow::NewDataAssetWindow(const WindowParams& InParams)
    : DialogWindow(InParams) {
}

NewDataAssetWindow* NewDataAssetWindow::Open(const String& InDirectory, const String& InName, CreatedFn InCreated) {
    Window* opener = CurrentOpener();

    SharedObjectPtr<NewDataAssetWindow> window(new NewDataAssetWindow(MakeParams("New Data Asset", 420, 390)));
    window->m_Directory = InDirectory;
    window->m_Name = InName;
    window->m_Created = std::move(InCreated);
    window->BuildContent();

    Present(window, opener);
    return window.Get();
}

void NewDataAssetWindow::BuildContent() {
    BuildFrame();

    AddCaption("Class");

    m_ClassPicker = AddFillPanel()->Add<UIClassPicker>();
    m_ClassPicker->Fill();
    m_ClassPicker->RootClass = DataAsset::StaticClass();
    for (const Class& candidate : Class::GetSubclassesOf(DataAsset::StaticClass())) {
        if (candidate.IsDefaultConstructable()) {
            m_ClassPicker->Selected = candidate;
            break;
        }
    }
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

void NewDataAssetWindow::Create() {
    const Class selected = m_ClassPicker->Selected;
    if (!selected.IsSubclassOf(DataAsset::StaticClass()) || !selected.IsDefaultConstructable()) {
        AE_WARN("Pick a class derived from DataAsset, in C++ or C#");
        return;
    }
    const String name = SanitizeName(m_Name);
    if (name.empty()) {
        AE_WARN("A Data Asset needs a name");
        return;
    }
    const String path = m_Directory + "/" + name + ".asset";
    if (std::filesystem::exists(path)) {
        AE_WARN("'{0}' already exists", path);
        return;
    }

    Asset* asset = AssetManager::Get().CreateAsset(selected, m_Directory, name);
    if (!asset) {
        return;
    }
    AssetManager::Get().SaveAsset(asset);

    if (m_Created) {
        m_Created(asset);
    }
    Close();
}

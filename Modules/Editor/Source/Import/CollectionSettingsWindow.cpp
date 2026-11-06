#include "CollectionSettingsWindow.h"

#include "Tabs/DetailsTab.h"
#include "Assets/AssetManager.h"
#include "Assets/CollectionAsset.h"
#include "Rendering/RenderingAPI.h"

CollectionSettingsWindow::CollectionSettingsWindow(const WindowParams& InParams)
    : DialogWindow(InParams) {
}

void CollectionSettingsWindow::Open(CollectionAsset* InCollection, std::function<void()> InReimported) {
    if (!InCollection) {
        return;
    }
    Window* opener = CurrentOpener();

    SharedObjectPtr<CollectionSettingsWindow> window(
        new CollectionSettingsWindow(MakeParams(InCollection->GetDisplayName() + " Import Settings", 480, 460)));
    window->m_Collection = InCollection;
    window->m_Edited = Object::Create<CollectionAsset>();
    window->m_Edited->DeserializeFromJson(InCollection->SerializeToJson());
    window->m_Reimported = std::move(InReimported);
    window->BuildContent();

    Present(window, opener);
}

bool CollectionSettingsWindow::Reimport(CollectionAsset& InCollection) {
    RenderingAPI::GetInstance()->WaitIdle();
    if (!InCollection.Reimport()) {
        return false;
    }
    return AssetManager::Get().SaveAsset(&InCollection);
}

void CollectionSettingsWindow::BuildContent() {
    BuildFrame();

    AddCaption("Sub-assets keep their edits. The ones the source no longer has are removed.");

    DetailsTab* details = AddFillPanel()->Add<DetailsTab>();
    details->Fill();
    details->Inspect(m_Edited.Get());

    AddFooterButton("Cancel", false, [this] { Close(); });
    AddFooterButton("Reimport", true, [this] { Apply(); });
}

void CollectionSettingsWindow::Apply() {
    CollectionAsset* collection = m_Collection.Get();
    if (!collection) {
        Close();
        return;
    }
    collection->SetSourcePath(m_Edited->GetSourcePath());
    collection->SetImportSettings(m_Edited->GetImportSettings());
    if (Reimport(*collection) && m_Reimported) {
        m_Reimported();
    }
    Close();
}

#pragma once
#include "DialogWindow.h"
#include "Object/Pointer.h"
#include "CollectionSettingsWindow.gen.h"

class CollectionAsset;

/** Edits a copy of a collection's import settings. */
class CollectionSettingsWindow : public DialogWindow {
public:
    ARTIFACT_CLASS();
protected:
    CollectionSettingsWindow(const WindowParams& InParams);
public:
    static void Open(CollectionAsset* InCollection, std::function<void()> InReimported);

    /** Applies the collection's current settings again, keeping the GPU idle while meshes are swapped. */
    static bool Reimport(CollectionAsset& InCollection);

private:
    void BuildContent();
    void Apply();

    WeakObjectPtr<CollectionAsset> m_Collection;
    SharedObjectPtr<CollectionAsset> m_Edited;
    std::function<void()> m_Reimported;
};

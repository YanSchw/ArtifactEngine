#pragma once
#include "DialogWindow.h"
#include "Object/Pointer.h"
#include "ImportWindow.gen.h"

class Asset;
class AssetImportMenu;
class DetailsTab;

/** Works through the import queue one file at a time: pick how to import it, adjust the settings,
 *  import. Import All reuses the settings for every queued file of the same extension. */
class ImportWindow : public DialogWindow {
public:
    ARTIFACT_CLASS();
protected:
    ImportWindow(const WindowParams& InParams);
public:
    virtual ~ImportWindow();

    using ImportedFn = std::function<void(Asset*)>;

    /** Queues files to be imported into InTargetDir, opening the dialog if it is not open yet. */
    static void Enqueue(const Array<String>& InFiles, const String& InTargetDir, ImportedFn InImported);

private:
    struct QueuedFile {
        String Path;
        String TargetDir;
    };

    void BuildContent();
    void ShowCurrent();
    void CreateSettings();
    AssetImportMenu* GetMenu() const;

    void ImportCurrent();
    void ImportAll();
    void Skip();

    static Asset* Import(const QueuedFile& InFile, const AssetImportMenu& InMenu, const String& InSettings);

    inline static Array<QueuedFile> s_Queue;
    inline static WeakObjectPtr<ImportWindow> s_Window;
    inline static ImportedFn s_Imported;

    Array<AssetImportMenu*> m_Menus;
    int32_t m_MenuIndex = 0;
    SharedObjectPtr<Asset> m_Settings;
    DetailsTab* m_Details = nullptr;
};

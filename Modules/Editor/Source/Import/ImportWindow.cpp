#include "ImportWindow.h"
#include "AssetImportMenus.h"

#include "Tabs/DetailsTab.h"
#include "UI/EditorIcons.h"
#include "UI/EditorStyle.h"
#include "UI/UIDropdown.h"
#include "Assets/Asset.h"
#include "Assets/AssetManager.h"
#include "Core/EngineConfig.h"
#include "Core/Log.h"
#include "GameFramework/UILabel.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace fs = std::filesystem;

/** What the user chose last time, per extension and per menu, so the next file starts from it. */
static Map<String, String> s_LastMenuByExtension;
static Map<String, String> s_LastSettingsByMenu;

static String GetExtension(const String& InPath) {
    String extension = fs::path(InPath).extension().string();
    std::ranges::transform(extension, extension.begin(), [](unsigned char InChar) { return (char)std::tolower(InChar); });
    return extension;
}

static bool HaveSameContents(const fs::path& InA, const fs::path& InB) {
    std::error_code error;
    if (fs::file_size(InA, error) != fs::file_size(InB, error) || error) {
        return false;
    }
    std::ifstream a(InA, std::ios::binary);
    std::ifstream b(InB, std::ios::binary);
    return std::equal(std::istreambuf_iterator<char>(a), std::istreambuf_iterator<char>(),
                      std::istreambuf_iterator<char>(b), std::istreambuf_iterator<char>());
}

static fs::path MakeUniquePath(const fs::path& InPath) {
    fs::path path = InPath;
    for (int32_t suffix = 2; fs::exists(path); suffix++) {
        path = InPath.parent_path() / (InPath.stem().string() + std::to_string(suffix) + InPath.extension().string());
    }
    return path;
}

/** Copies the source and the files it reads into the target directory, keeping their layout
 *  relative to the source so its references still resolve. Returns where the source landed. */
static fs::path CopyIntoProject(const fs::path& InSource, const Array<String>& InFiles, const fs::path& InTargetDir) {
    std::error_code error;
    fs::create_directories(InTargetDir, error);

    fs::path source = InTargetDir / InSource.filename();
    if (fs::exists(source) && !HaveSameContents(InSource, source)) {
        source = MakeUniquePath(source);
    }
    if (!fs::exists(source) && !fs::copy_file(InSource, source, error)) {
        AE_ERROR("Failed to copy '{0}' into '{1}': {2}", InSource.string(), InTargetDir.string(), error.message());
        return fs::path();
    }

    for (const String& file : InFiles) {
        const fs::path path = fs::path(file).lexically_normal();
        if (path == InSource) {
            continue;
        }
        const fs::path relative = path.lexically_relative(InSource.parent_path());
        if (relative.empty() || *relative.begin() == "..") {
            AE_WARN("'{0}' is outside the folder of '{1}' and was not copied", path.string(), InSource.string());
            continue;
        }
        const fs::path target = InTargetDir / relative;
        if (fs::exists(target)) {
            if (!HaveSameContents(path, target)) {
                AE_WARN("Kept the existing '{0}', which differs from '{1}'", target.string(), path.string());
            }
            continue;
        }
        fs::create_directories(target.parent_path(), error);
        fs::copy_file(path, target, error);
    }
    return source;
}

ImportWindow::ImportWindow(const WindowParams& InParams)
    : DialogWindow(InParams) {
}

ImportWindow::~ImportWindow() {
    s_Queue.Clear();
}

void ImportWindow::Enqueue(const Array<String>& InFiles, const String& InTargetDir, ImportedFn InImported) {
    for (const String& file : InFiles) {
        if (!fs::exists(file)) {
            AE_ERROR("Cannot import '{0}': the file does not exist", file);
        } else if (fs::is_directory(file)) {
            AE_ERROR("Cannot import the folder '{0}'; import the files inside it instead", file);
        } else if (AssetImportMenu::FindFor(file).IsEmpty()) {
            AE_ERROR("Cannot import '{0}': nothing imports '{1}' files", file, GetExtension(file));
        } else {
            s_Queue.Add({ fs::absolute(file).lexically_normal().string(), InTargetDir });
        }
    }
    s_Imported = std::move(InImported);

    if (s_Queue.IsEmpty() || s_Window.Get()) {
        return;
    }
    Window* opener = CurrentOpener();
    SharedObjectPtr<ImportWindow> window(new ImportWindow(MakeParams("Import", 480, 560)));
    window->BuildContent();
    window->ShowCurrent();
    s_Window = window.Get();
    Present(window, opener);
}

void ImportWindow::BuildContent() {
    BuildFrame();

    UILabel* file = AddCaption("");
    file->Color = EditorStyle::TextBright;
    file->Bind = [file] {
        const String queued = s_Queue.Size() > 1 ? "   (" + std::to_string(s_Queue.Size() - 1) + " more queued)" : String();
        file->Text = s_Queue.IsEmpty() ? String() : fs::path(s_Queue[0].Path).filename().string() + queued;
    };

    UIDropdown* menus = AddNamedRow("Import As")->Add<UIDropdown>();
    menus->Fill();
    menus->GetSelectedLabel = [this] {
        AssetImportMenu* menu = GetMenu();
        return menu ? menu->GetName() : String();
    };
    menus->GetOptions = [this] {
        Array<UIDropdownOption> options;
        for (AssetImportMenu* menu : m_Menus) {
            UIDropdownOption option(menu->GetName());
            option.Icon = EditorIcons::GetAssetIcon(menu->GetAssetClass());
            option.IconTint = EditorIcons::GetAssetColor(menu->GetAssetClass());
            options.Add(option);
        }
        return options;
    };
    menus->GetSelectedIndex = [this] { return m_MenuIndex; };
    menus->SelectionChanged = [this](int32_t InIndex) {
        m_MenuIndex = InIndex;
        CreateSettings();
    };

    m_Details = AddFillPanel()->Add<DetailsTab>();
    m_Details->Fill();

    AddFooterButton("Skip", false, [this] { Skip(); });
    AddFooterButton("Import All", false, [this] { ImportAll(); });
    AddFooterButton("Import", true, [this] { ImportCurrent(); });
}

AssetImportMenu* ImportWindow::GetMenu() const {
    return m_MenuIndex >= 0 && m_MenuIndex < m_Menus.Size() ? m_Menus[m_MenuIndex] : nullptr;
}

void ImportWindow::ShowCurrent() {
    if (s_Queue.IsEmpty()) {
        Close();
        return;
    }
    const String& path = s_Queue[0].Path;
    m_Menus = AssetImportMenu::FindFor(path);
    m_MenuIndex = 0;
    const String lastMenu = s_LastMenuByExtension.GetOrDefault(GetExtension(path), String());
    for (int32_t i = 0; i < m_Menus.Size(); i++) {
        if (m_Menus[i]->GetClass().Name == lastMenu) {
            m_MenuIndex = i;
        }
    }
    CreateSettings();
}

void ImportWindow::CreateSettings() {
    AssetImportMenu* menu = GetMenu();
    m_Settings = menu ? Cast<Asset>(Object::Create(menu->GetAssetClass())) : nullptr;
    if (Asset* settings = m_Settings.Get()) {
        const String lastSettings = s_LastSettingsByMenu.GetOrDefault(menu->GetClass().Name, String());
        if (!lastSettings.empty()) {
            settings->DeserializeFromJson(lastSettings);
        }
        settings->SetSourcePath(s_Queue[0].Path);
    }
    m_Details->Inspect(m_Settings.Get());
}

Asset* ImportWindow::Import(const QueuedFile& InFile, const AssetImportMenu& InMenu, const String& InSettings) {
    const fs::path source = InFile.Path;
    fs::path inProject = source;
    if (EngineConfig::ToContentPath(source.string()).empty()) {
        inProject = CopyIntoProject(source, InMenu.GetSourceFiles(source.string()), InFile.TargetDir);
    }
    if (inProject.empty()) {
        return nullptr;
    }
    const String contentPath = EngineConfig::ToContentPath(inProject.string());
    if (contentPath.empty()) {
        AE_ERROR("Cannot import '{0}': '{1}' is not inside a content folder", source.string(), InFile.TargetDir);
        return nullptr;
    }

    Asset* asset = Cast<Asset>(Object::Create(InMenu.GetAssetClass()));
    asset->DeserializeFromJson(InSettings);
    const String name = MakeUniquePath(fs::path(InFile.TargetDir) / (inProject.stem().string() + ".asset")).stem().string();
    AssetManager::Get().AddAsset(asset, InFile.TargetDir, name);
    asset->SetSourcePath(contentPath);
    InMenu.OnImported(*asset);
    AssetManager::Get().LoadAsset(asset);
    AssetManager::Get().SaveAsset(asset);

    AE_INFO("Imported '{0}' as {1} '{2}'", source.filename().string(), InMenu.GetName(), name);
    if (s_Imported) {
        s_Imported(asset);
    }
    return asset;
}

void ImportWindow::ImportCurrent() {
    AssetImportMenu* menu = GetMenu();
    if (s_Queue.IsEmpty() || !menu || !m_Settings.Get()) {
        return;
    }
    const QueuedFile file = s_Queue[0];
    s_Queue.RemoveAt(0);

    const String settings = m_Settings->SerializeToJson();
    s_LastMenuByExtension[GetExtension(file.Path)] = menu->GetClass().Name;
    s_LastSettingsByMenu[menu->GetClass().Name] = settings;
    Import(file, *menu, settings);
    ShowCurrent();
}

void ImportWindow::ImportAll() {
    AssetImportMenu* menu = GetMenu();
    if (s_Queue.IsEmpty() || !menu || !m_Settings.Get()) {
        return;
    }
    const String extension = GetExtension(s_Queue[0].Path);
    const String settings = m_Settings->SerializeToJson();
    s_LastMenuByExtension[extension] = menu->GetClass().Name;
    s_LastSettingsByMenu[menu->GetClass().Name] = settings;

    for (int32_t i = 0; i < s_Queue.Size();) {
        if (GetExtension(s_Queue[i].Path) != extension) {
            i++;
            continue;
        }
        const QueuedFile file = s_Queue[i];
        s_Queue.RemoveAt(i);
        Import(file, *menu, settings);
    }
    ShowCurrent();
}

void ImportWindow::Skip() {
    if (!s_Queue.IsEmpty()) {
        s_Queue.RemoveAt(0);
    }
    ShowCurrent();
}

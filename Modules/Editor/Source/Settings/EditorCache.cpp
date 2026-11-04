#include "EditorCache.h"
#include "EditorWindow.h"
#include "Tabs/MajorTab.h"
#include "Tabs/EditorSettingsTab.h"
#include "Assets/Asset.h"
#include "Assets/AssetManager.h"
#include "Core/EngineConfig.h"
#include "Platform/FileIO.h"
#include "Serialization/Json.h"
#include <filesystem>

static float DistanceToWorkArea(const MonitorInfo& InMonitor, const Vec2& InPoint) {
    const Vec2 closest = glm::clamp(InPoint, InMonitor.WorkAreaPosition, InMonitor.WorkAreaPosition + InMonitor.WorkAreaSize);
    return glm::length(InPoint - closest);
}

static const MonitorInfo* FindMonitorAt(const Array<MonitorInfo>& InMonitors, const Vec2& InPoint) {
    const MonitorInfo* nearest = nullptr;
    for (const MonitorInfo& monitor : InMonitors) {
        if (!nearest || DistanceToWorkArea(monitor, InPoint) < DistanceToWorkArea(*nearest, InPoint)) {
            nearest = &monitor;
        }
    }
    return nearest;
}

static const MonitorInfo* FindRecordedMonitor(const Array<MonitorInfo>& InMonitors, const EditorWindowPlacement& InPlacement) {
    const MonitorInfo* match = nullptr;
    for (const MonitorInfo& monitor : InMonitors) {
        if (monitor.Name == InPlacement.Monitor && (!match || monitor.Position == InPlacement.MonitorPosition)) {
            match = &monitor;
        }
    }
    return match ? match : (InMonitors.IsEmpty() ? nullptr : &InMonitors[0]);
}

static EditorWindowPlacement CapturePlacement(const Window& InWindow) {
    EditorWindowPlacement placement;
    placement.Size = InWindow.GetRestoredSize();
    placement.Maximized = InWindow.IsMaximized();

    const Array<MonitorInfo> monitors = Window::GetMonitors();
    const Vec2 center = InWindow.GetPosition() + Vec2((float)InWindow.GetWidth(), (float)InWindow.GetHeight()) * 0.5f;
    if (const MonitorInfo* monitor = FindMonitorAt(monitors, center)) {
        placement.Monitor = monitor->Name;
        placement.MonitorPosition = monitor->Position;
    }
    placement.Offset = InWindow.GetRestoredPosition() - placement.MonitorPosition;
    return placement;
}

static void ApplyPlacement(Window& InWindow, const EditorWindowPlacement& InPlacement) {
    if (InPlacement.Size.x <= 0.0f || InPlacement.Size.y <= 0.0f) {
        return;
    }
    const Array<MonitorInfo> monitors = Window::GetMonitors();
    const MonitorInfo* monitor = FindRecordedMonitor(monitors, InPlacement);
    if (!monitor || monitor->WorkAreaSize.x <= 0.0f || monitor->WorkAreaSize.y <= 0.0f) {
        return;
    }

    // Keep the whole window on its monitor, also when the recorded one went missing or shrank.
    const Vec2 areaMin = monitor->WorkAreaPosition;
    const Vec2 size = glm::min(InPlacement.Size, monitor->WorkAreaSize);
    const Vec2 areaMax = areaMin + monitor->WorkAreaSize - size;
    InWindow.SetSize(size);
    InWindow.SetPosition(glm::clamp(monitor->Position + InPlacement.Offset, areaMin, areaMax));
    if (InPlacement.Maximized) {
        InWindow.Maximize();
    }
}

static SharedObjectPtr<EditorWindow> CreatePlacedWindow(const EditorWindowPlacement* InPlacement) {
    SharedObjectPtr<EditorWindow> window = EditorWindow::Create(WindowParams{ "Artifact Editor", 1280, 720 });
    ApplyPlacement(*window, InPlacement ? *InPlacement : CapturePlacement(*window));
    return window;
}

static bool IsReopenable(const EditorTabRecord& InRecord) {
    if (InRecord.AssetId.IsValid()) {
        return AssetManager::Get().GetAsset(InRecord.AssetId) != nullptr;
    }
    return InRecord.TabClass == EditorSettingsTab::StaticClass().Name;
}

static MajorTab* ReopenTab(EditorWindow& InWindow, const EditorTabRecord& InRecord) {
    if (!IsReopenable(InRecord)) {
        return nullptr;
    }
    if (InRecord.AssetId.IsValid()) {
        return InWindow.OpenAssetEditor(AssetManager::Get().GetAsset(InRecord.AssetId));
    }
    return InWindow.OpenTab<EditorSettingsTab>();
}

static void ReopenWindowTabs(EditorWindow& InWindow, const EditorWindowRecord& InRecord) {
    MajorTab* active = nullptr;
    for (int32_t i = 0; i < InRecord.Tabs.Size(); i++) {
        MajorTab* tab = ReopenTab(InWindow, InRecord.Tabs[i]);
        if (tab && (i == InRecord.ActiveTab || !active)) {
            active = tab;
        }
    }
    if (active) {
        InWindow.ActivateTab(active);
    }
}

EditorCache& EditorCache::Get() {
    static EditorCache s_Cache;
    return s_Cache;
}

String EditorCache::GetFilePath() {
    return (std::filesystem::path(EngineConfig::GetProjectDir()) / "Saved" / "EditorCache.json").string();
}

void EditorCache::Load() {
    const String path = GetFilePath();
    if (FileIO::FileExists(path) && !JsonSerializer::TryDeserializeObject(this, FileIO::ReadFileToString(path))) {
        AE_WARN("Ignoring unreadable editor cache at {0}", path);
        Windows.Clear();
    }
}

void EditorCache::Save() const {
    const std::filesystem::path path = GetFilePath();
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (!FileIO::WriteStringToFile(path.string(), JsonSerializer::SerializeObject(this))) {
        AE_WARN("Failed to save the editor cache to {0}", path.string());
    }
}

void EditorCache::Capture() {
    Windows.Clear();
    for (const SharedObjectPtr<ThemedWindow>& window : ThemedWindow::GetAllWindows()) {
        EditorWindow* editorWindow = window.Get() ? window->As<EditorWindow>() : nullptr;
        if (!editorWindow) {
            continue;
        }
        EditorWindowRecord record;
        record.Placement = CapturePlacement(*editorWindow);
        for (MajorTab* tab : editorWindow->GetOpenTabs()) {
            EditorTabRecord tabRecord;
            tabRecord.TabClass = tab->GetClass().Name;
            if (Asset* asset = tab->GetEditedAsset()) {
                tabRecord.AssetId = asset->GetId();
            } else if (!tab->IsA<EditorSettingsTab>()) {
                continue;
            }
            if (tab == editorWindow->GetActiveTab()) {
                record.ActiveTab = record.Tabs.Size();
            }
            record.Tabs.Add(tabRecord);
        }
        Windows.Add(record);
    }
}

SharedObjectPtr<EditorWindow> EditorCache::CreateMainWindow() const {
    return CreatePlacedWindow(Windows.IsEmpty() ? nullptr : &Windows[0].Placement);
}

bool EditorCache::ReopenTabs(EditorWindow& InMainWindow) const {
    for (int32_t i = 0; i < Windows.Size(); i++) {
        const EditorWindowRecord& record = Windows[i];
        if (i == 0) {
            ReopenWindowTabs(InMainWindow, record);
            continue;
        }
        bool hasTabs = false;
        for (const EditorTabRecord& tab : record.Tabs) {
            hasTabs = hasTabs || IsReopenable(tab);
        }
        if (hasTabs) {
            ReopenWindowTabs(*CreatePlacedWindow(&record.Placement), record);
        }
    }
    return !InMainWindow.GetOpenTabs().IsEmpty();
}

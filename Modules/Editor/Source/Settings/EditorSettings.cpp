#include "EditorSettings.h"
#include "EditorCamera.h"
#include "UI/EditorStyle.h"
#include "Assets/AssetManager.h"
#include "Assets/Font.h"
#include "GameFramework/UINode.h"
#include "Platform/Platform.h"
#include "Platform/FileIO.h"
#include "Serialization/Json.h"
#include <algorithm>
#include <filesystem>

EditorSettings& EditorSettings::Get() {
    static EditorSettings s_Settings;
    return s_Settings;
}

String EditorSettings::GetFilePath() {
    return (std::filesystem::path(Platform::GetUserConfigDirectory()) / "ArtifactEngine" / "EditorSettings.json").string();
}

void EditorSettings::Load() {
    const String path = GetFilePath();
    if (!FileIO::FileExists(path)) {
        return;
    }
    if (!JsonSerializer::TryDeserializeObject(this, FileIO::ReadFileToString(path))) {
        AE_WARN("Ignoring unreadable editor settings at {0}", path);
        ResetToDefaults();
    }
    Sanitize();
}

void EditorSettings::Save() const {
    const std::filesystem::path path = GetFilePath();
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (!FileIO::WriteStringToFile(path.string(), JsonSerializer::SerializeObject(this))) {
        AE_WARN("Failed to save the editor settings to {0}", path.string());
    }
}

void EditorSettings::ResetToDefaults() {
    General = EditorGeneralSettings();
    Appearance = EditorAppearanceSettings();
    Viewport = EditorViewportSettings();
    Gizmos = EditorGizmoSettings();
}

void EditorSettings::Sanitize() {
    if (Appearance.Theme > EditorTheme::Contrast) {
        Appearance.Theme = EditorTheme::Dark;
    }
    Appearance.InterfaceScale = std::clamp(Appearance.InterfaceScale, 0.5f, 3.0f);
    Appearance.FontSize = std::clamp(Appearance.FontSize, 10.0f, 18.0f);
    Viewport.LookSensitivity = std::clamp(Viewport.LookSensitivity, 0.01f, 5.0f);
    Viewport.DefaultFlySpeed = std::clamp(Viewport.DefaultFlySpeed, EditorCamera::MinFlySpeed, EditorCamera::MaxFlySpeed);
}

void EditorSettings::ApplyStartupSettings() {
    EditorStyle::Apply(Appearance);
    m_StartupTheme = Appearance.Theme;
    m_StartupFontSize = Appearance.FontSize;
}

void EditorSettings::ApplyInterfaceFont() const {
    Font* font = AssetManager::Get().GetAsset<Font>(Appearance.InterfaceFont);
    if (!font) {
        font = AssetManager::Get().GetAsset<Font>(EditorAppearanceSettings().InterfaceFont);
    }
    AssetManager::Get().LoadAsset(font);
    UINode::SetDefaultFont(font);
}

bool EditorSettings::NeedsRestart() const {
    return Appearance.Theme != m_StartupTheme || Appearance.FontSize != m_StartupFontSize;
}

#pragma once
#include "Object/Object.h"
#include "Object/Enum.h"
#include "Common/UUID.h"
#include "EditorSettings.gen.h"

ARTIFACT_ENUM();
enum class EditorTheme : uint8_t {
    Dark = 0,
    Graphite,
    Contrast
};

struct EditorGeneralSettings {
    ARTIFACT_STRUCT();

    PROPERTY()
    bool ReopenTabs = true;

    PROPERTY()
    bool ShowFrameStats = true;
};

struct EditorAppearanceSettings {
    ARTIFACT_STRUCT();

    PROPERTY()
    EditorTheme Theme = EditorTheme::Dark;

    PROPERTY()
    float InterfaceScale = 1.0f;

    PROPERTY()
    UUID InterfaceFont = UUID::FromString("3e50e8a9-df79-4e81-89be-189a15ca838b");

    PROPERTY()
    float FontSize = 13.0f;
};

struct EditorViewportSettings {
    ARTIFACT_STRUCT();

    PROPERTY()
    float LookSensitivity = 0.35f;

    PROPERTY()
    float DefaultFlySpeed = 5.0f;
};

/** Editor-only preferences shared by every project, stored in the user's config directory. */
class EditorSettings : public Object {
public:
    ARTIFACT_CLASS();

    static EditorSettings& Get();
    static String GetFilePath();

    PROPERTY()
    EditorGeneralSettings General;

    PROPERTY()
    EditorAppearanceSettings Appearance;

    PROPERTY()
    EditorViewportSettings Viewport;

    void Load();
    void Save() const;
    void ResetToDefaults();
    void Sanitize();

    /** Applies the settings the chrome is built from; call once, before the first window. */
    void ApplyStartupSettings();
    void ApplyInterfaceFont() const;
    /** True when a setting only read by ApplyStartupSettings() changed since. */
    bool NeedsRestart() const;

private:
    EditorTheme m_StartupTheme = EditorTheme::Dark;
    float m_StartupFontSize = 13.0f;
};

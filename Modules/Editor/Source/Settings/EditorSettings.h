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

    PROPERTY(Range = (0.5, 3))
    float InterfaceScale = 1.0f;

    PROPERTY()
    UUID InterfaceFont = UUID::FromString("3e50e8a9-df79-4e81-89be-189a15ca838b");

    PROPERTY(Range = (10, 18))
    float FontSize = 13.0f;
};

struct EditorViewportSettings {
    ARTIFACT_STRUCT();

    PROPERTY(Range = (0.01, 5))
    float LookSensitivity = 0.35f;

    PROPERTY(Range = (0.25, 100))
    float DefaultFlySpeed = 5.0f;
};

struct EditorGizmoSettings {
    ARTIFACT_STRUCT();

    PROPERTY()
    bool ShowGizmos = true;

    PROPERTY()
    bool Cameras = true;

    PROPERTY()
    bool DirectionalLights = true;

    PROPERTY()
    bool PointLights = true;

    PROPERTY()
    bool AudioSources = true;

    PROPERTY()
    bool AudioListeners = true;

    PROPERTY()
    bool Colliders = true;

    PROPERTY()
    bool Characters = true;

    PROPERTY()
    bool PostProcessVolumes = true;
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

    PROPERTY()
    EditorGizmoSettings Gizmos;

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

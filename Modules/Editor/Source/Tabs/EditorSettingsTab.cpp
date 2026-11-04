#include "EditorSettingsTab.h"
#include "DetailsTab.h"
#include "Settings/EditorSettings.h"
#include "UI/UIDockArea.h"
#include "UI/EditorIcons.h"
#include "UI/EditorStyle.h"

EditorSettingsTab::EditorSettingsTab() {
    GetDockArea()->DockNew<DetailsTab>(UIDockSlot::Center);
    SetSelection(&EditorSettings::Get());
}

VectorImage* EditorSettingsTab::GetTabIcon() const {
    return EditorIcons::Settings();
}

void EditorSettingsTab::BuildToolBar(UINode& InToolBar) {
    EditorStyle::IconButton(InToolBar, EditorIcons::Reset(), EditorStyle::Text, "Reset to Defaults", 136.0f,
                            [this] { ResetToDefaults(); });
}

void EditorSettingsTab::ResetToDefaults() {
    EditorSettings& settings = EditorSettings::Get();
    BeginTransaction("Reset Editor Settings", &settings);
    settings.ResetToDefaults();
    EndTransaction();
    OnObjectEdited(&settings);
}

void EditorSettingsTab::OnObjectEdited(Object* InObject) {
    EditorSettings& settings = EditorSettings::Get();
    if (InObject != &settings) {
        return;
    }
    settings.Sanitize();
    settings.ApplyInterfaceFont();
    settings.Save();
}

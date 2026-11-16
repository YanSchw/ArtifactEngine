#include "ProjectSettingsTab.h"
#include "DetailsTab.h"
#include "Settings/ProjectConfig.h"
#include "Core/ProjectSettings.h"
#include "UI/UIDockArea.h"
#include "UI/EditorIcons.h"

ProjectSettingsTab::ProjectSettingsTab() {
    ProjectConfig::Get().Load();
    GetDockArea()->DockNew<DetailsTab>(UIDockSlot::Center);
    SetSelection(&ProjectConfig::Get());
}

VectorImage* ProjectSettingsTab::GetTabIcon() const {
    return EditorIcons::Settings();
}

void ProjectSettingsTab::OnObjectEdited(Object* InObject) {
    if (Cast<ProjectSettings>(InObject)) {
        ProjectConfig::Get().Save();
    }
}

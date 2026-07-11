#include "SceneEditorTab.h"
#include "OutlinerTab.h"
#include "DetailsTab.h"
#include "ViewportTab.h"
#include "UI/UIDockArea.h"
#include "UI/EditorStyle.h"
#include "GameFramework/UIBuilder.h"
#include "Core/Log.h"

SceneEditorTab::SceneEditorTab() {
    UIDockArea* area = GetDockArea();
    area->DockNew<ViewportTab>(UIDockSlot::Center);
    OutlinerTab* outliner = area->DockNew<OutlinerTab>(UIDockSlot::Left, nullptr, 0.24f);
    area->DockNew<DetailsTab>(UIDockSlot::Bottom, outliner->GetDockNode(), 0.45f);
}

void SceneEditorTab::BuildToolBar(UINode& InToolBar) {
    const auto addButton = [&InToolBar](const String& InCaption, std::function<void()> InAction) {
        UIButton& button = UI::Button(InToolBar, InCaption, std::move(InAction));
        button.Size = { 70.0_px, 1.0_rel };
        EditorStyle::ApplyButtonStyle(button);
    };
    addButton("Save", [] { AE_INFO("SceneEditorTab: Save"); });
    addButton("Settings", [] { AE_INFO("SceneEditorTab: Settings"); });
    addButton("Play", [] { AE_INFO("SceneEditorTab: Play"); });
}

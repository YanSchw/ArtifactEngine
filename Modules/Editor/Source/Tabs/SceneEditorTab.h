#pragma once
#include "MajorTab.h"
#include "SceneEditorTab.gen.h"

/** The scene editor: viewport in the center, outliner and details on the left. */
class SceneEditorTab : public MajorTab {
public:
    ARTIFACT_CLASS();

    SceneEditorTab();

    virtual String GetTabTitle() const override { return "MyLevel"; }
    virtual void BuildToolBar(UINode& InToolBar) override;
};

#pragma once
#include "MajorTab.h"
#include "EditorSettingsTab.gen.h"

/** Edits the EditorSettings in a details panel, saving every change right away. */
class EditorSettingsTab : public MajorTab {
public:
    ARTIFACT_CLASS();

    EditorSettingsTab();

    virtual String GetTabTitle() const override { return "Editor Settings"; }
    virtual VectorImage* GetTabIcon() const override;
    virtual void BuildToolBar(UINode& InToolBar) override;
    virtual void OnObjectEdited(Object* InObject) override;

    void ResetToDefaults();
};

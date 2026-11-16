#pragma once
#include "MajorTab.h"
#include "ProjectSettingsTab.gen.h"

class ProjectSettingsTab : public MajorTab {
public:
    ARTIFACT_CLASS();

    ProjectSettingsTab();

    virtual String GetTabTitle() const override { return "Project Settings"; }
    virtual VectorImage* GetTabIcon() const override;
    virtual void OnObjectEdited(Object* InObject) override;
};

#pragma once
#include "DetailsCustomization.h"
#include "Settings/ProjectConfig.h"
#include "ProjectSettingsDetailsCustomization.gen.h"

class ProjectSettings;

class ProjectSettingsDetailsCustomization : public DetailsCustomization {
public:
    ARTIFACT_CLASS();

    virtual Class GetSupportedClass() const override { return ProjectConfig::StaticClass(); }
    virtual float BuildHeader(UINode& InHeader, Object* InObject, DetailsTab& InTab) override;
    virtual void BuildContent(UINode& InList, Object* InObject, DetailsTab& InTab) override;

private:
    static void BindSettingsRow(DetailsRow& InRow, DetailsTab& InTab, ProjectSettings* InSettings, Property* InProperty);
    static void AddFreezeButton(DetailsRow& InRow, DetailsTab& InTab, const String& InName);
};

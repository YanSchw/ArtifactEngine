#pragma once
#include "DetailsCustomization.h"
#include "Settings/EditorSettings.h"
#include "EditorSettingsDetailsCustomization.gen.h"

/** Lists each settings group as its own category and picks the interface font from the font assets. */
class EditorSettingsDetailsCustomization : public DetailsCustomization {
public:
    ARTIFACT_CLASS();

    virtual Class GetSupportedClass() const override { return EditorSettings::StaticClass(); }
    virtual float BuildHeader(UINode& InHeader, Object* InObject, DetailsTab& InTab) override;
    virtual void BuildContent(UINode& InList, Object* InObject, DetailsTab& InTab) override;

private:
    static void AddFontRow(UINode& InParent, DetailsTab& InTab, const WeakObjectPtr<Object>& InObject, Property* InProperty);
};

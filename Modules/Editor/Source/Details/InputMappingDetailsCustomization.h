#pragma once
#include "DetailsCustomization.h"
#include "InputSystem/InputAction.h"
#include "InputSystem/InputBinding.h"
#include "InputMappingDetailsCustomization.gen.h"

/** Records edits on the open mapping rather than the inspected action or binding, because undo
 *  restores the mapping wholesale and with it replaces every action and binding it owns. */
class InputMappingDetailsCustomization : public DetailsCustomization {
public:
    ARTIFACT_CLASS();

protected:
    static DetailsEditHandler MakeMappingEditHandler(DetailsTab& InTab, Object* InObject, Property* InProperty);
    static void AddValueRow(UINode& InParent, DetailsTab& InTab, Object* InObject, Property* InProperty);
};

class InputActionDetailsCustomization : public InputMappingDetailsCustomization {
public:
    ARTIFACT_CLASS();

    virtual Class GetSupportedClass() const override { return InputAction::StaticClass(); }
    virtual void BuildContent(UINode& InList, Object* InObject, DetailsTab& InTab) override;
};

class InputBindingDetailsCustomization : public InputMappingDetailsCustomization {
public:
    ARTIFACT_CLASS();

    virtual Class GetSupportedClass() const override { return InputBinding::StaticClass(); }
    virtual float BuildHeader(UINode& InHeader, Object* InObject, DetailsTab& InTab) override;
    virtual void BuildContent(UINode& InList, Object* InObject, DetailsTab& InTab) override;

private:
    static void AddControlPathRow(UINode& InParent, DetailsTab& InTab, Object* InObject, Property* InProperty);
};

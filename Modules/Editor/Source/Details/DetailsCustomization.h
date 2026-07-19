#pragma once
#include "Object/Object.h"
#include "Object/Property.h"
#include "Object/Pointer.h"
#include "Common/Array.h"
#include "DetailsCustomization.gen.h"

class DetailsTab;
class DetailsCategory;
class DetailsRow;
class UINode;

/** Builds the Details view for one class of inspected object. Subclassing is the registration:
 *  the panel picks the customization whose supported class is the closest base of the object. */
class DetailsCustomization : public Object {
public:
    ARTIFACT_CLASS();

    virtual Class GetSupportedClass() const { return Class::None; }
    virtual float BuildHeader(UINode& InHeader, Object* InObject, DetailsTab& InTab);
    /** Default: one category per class of the inheritance chain (base first), rows per PROPERTY. */
    virtual void BuildContent(UINode& InList, Object* InObject, DetailsTab& InTab);

    static DetailsCustomization* FindFor(const Class& InClass);

protected:
    virtual bool WantsClassCategory(const Class& InClass) const;
    virtual void BuildClassCategory(DetailsCategory& InCategory, const Class& InClass, Object* InObject, DetailsTab& InTab);

    static DetailsCategory& AddCategory(UINode& InParent, DetailsTab& InTab, const String& InTitle, int32_t InDepth);
    static DetailsRow& AddRow(UINode& InParent, DetailsTab& InTab, const String& InLabel, int32_t InDepth);
    static void AddPropertyRow(UINode& InParent, DetailsTab& InTab, const WeakObjectPtr<Object>& InObject,
                               uint64_t InBaseOffset, Property* InProperty, int32_t InDepth);
    static String PrettyClassName(const Class& InClass);
};

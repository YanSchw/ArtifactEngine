#pragma once
#include "Object/Object.h"
#include "Object/Property.h"
#include "Object/Pointer.h"
#include "Common/Array.h"
#include <functional>
#include "DetailsCustomization.gen.h"

class DetailsTab;
class DetailsCategory;
class DetailsRow;
class UINode;

struct DetailsEditHandler {
    std::function<void()> BeginEdit;
    std::function<void()> CommitEdit;

    void Record() const { if (BeginEdit) BeginEdit(); }
    void operator()() const { if (CommitEdit) CommitEdit(); }
};

/** Builds the Details view for one class of inspected object. Subclassing is the registration:
 *  the panel picks the customization whose supported class is the closest base of the object. */
class DetailsCustomization : public Object {
public:
    ARTIFACT_CLASS();

    virtual Class GetSupportedClass() const { return Class::None; }
    /** True when editing this property changes which rows the view should show. */
    virtual bool RebuildsOnEdit(const String& InPropertyName) const { (void)InPropertyName; return false; }
    virtual float BuildHeader(UINode& InHeader, Object* InObject, DetailsTab& InTab);
    /** Default: one category per class of the inheritance chain (base first), rows per PROPERTY. */
    virtual void BuildContent(UINode& InList, Object* InObject, DetailsTab& InTab);

    static DetailsCustomization* FindFor(const Class& InClass);

    /** InPropertyPath may reach into structs (see PropertyPath); the reset then only touches that member. */
    static void BindOverride(DetailsRow& InRow, const WeakObjectPtr<Object>& InObject, const String& InPropertyPath);
    /** Every edit becomes an undoable transaction titled after InPropertyPath (InRootProperty when empty),
     *  and marks that path overridden. */
    static DetailsEditHandler MakeEditHandler(const WeakObjectPtr<Object>& InObject, Property* InRootProperty, DetailsTab* InTab = nullptr,
                                              const String& InPropertyPath = String());
    /** The members a struct property is edited through, empty for anything edited as one value. */
    static Array<Property*> GetInnerProperties(Property* InProperty);
    /** Builds the widget one property is edited with into InHost. False for types that have none. */
    static bool BuildValueEditor(UINode& InHost, const WeakObjectPtr<Object>& InObject, uint64_t InOffset,
                                 Property* InProperty, const DetailsEditHandler& InOnEdited, const String& InTitle = String());
    /** Marks the override and tells the owning MajorTab. Every row that writes a property itself,
     *  instead of going through MakeEditHandler, has to end with this. InPropertyName may be a path. */
    static void NotifyPropertyEdited(DetailsTab* InTab, Object* InObject, const String& InPropertyName);
    static void NotifyPropertyEdited(class MajorTab* InTab, Object* InObject, const String& InPropertyName);
    static String PrettyClassName(const Class& InClass);
    static String PrettyPropertyName(const String& InName);
    /** "m_Params.Bloom.Intensity" reads "Bloom Intensity": the members below the root, or the root alone. */
    static String PrettyPropertyPath(const String& InPath);

protected:
    virtual bool WantsClassCategory(const Class& InClass) const;
    virtual void BuildClassCategory(DetailsCategory& InCategory, const Class& InClass, Object* InObject, DetailsTab& InTab);

    static DetailsCategory& AddCategory(UINode& InParent, DetailsTab& InTab, const String& InTitle, int32_t InDepth);
    static DetailsRow& AddRow(UINode& InParent, DetailsTab& InTab, const String& InLabel, int32_t InDepth);
    static void AddPropertyRow(UINode& InParent, DetailsTab& InTab, const WeakObjectPtr<Object>& InObject,
                               uint64_t InBaseOffset, Property* InProperty, int32_t InDepth,
                               Property* InRootProperty = nullptr, const String& InLabel = String(),
                               const String& InParentPath = String());
    static DetailsCategory& AddStructCategory(UINode& InParent, DetailsTab& InTab, const WeakObjectPtr<Object>& InObject,
                                              uint64_t InOffset, const Array<Property*>& InMembers, int32_t InDepth,
                                              Property* InRootProperty, const String& InLabel, const String& InPath);
    static void AddArrayRows(UINode& InParent, DetailsTab& InTab, const WeakObjectPtr<Object>& InObject, uint64_t InOffset,
                             ArrayProperty* InArray, int32_t InDepth, Property* InRootProperty,
                             const String& InLabel, const String& InPath);
    static void AddArrayElement(UINode& InParent, DetailsTab& InTab, const WeakObjectPtr<Object>& InObject, uint64_t InArrayOffset,
                                ArrayProperty* InArray, int32_t InIndex, int32_t InDepth, Property* InRootProperty,
                                const String& InArrayPath);
};

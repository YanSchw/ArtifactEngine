#include "Object.h"
#include "Pointer.h"
#include "Common/Map.h"

static Map<String, String> s_ChildToParentClassnames;

const Class Class::None = Class("");

Class Class::GetParentClass() const {
    return s_ChildToParentClassnames.At(Name);
}

bool Class::IsSubclassOf(const Class& InBaseClass) const {
    if ((*this) == InBaseClass) return true;
    return (Name == "Object") 
        ? false 
        : (
            GetParentClass() == InBaseClass
                ? true
                : (
                    GetParentClass() != Class::None 
                        ? GetParentClass().IsSubclassOf(InBaseClass)
                        : false
                )
        );
}

Class::RegisterParentChildClassRelationship::RegisterParentChildClassRelationship(const String& InChild, const String& InParent) {
    s_ChildToParentClassnames[InChild] = InParent;
}

Object* Object::Create(const Class& type) {
    if (s_ObjectAllocators.find(type.Name) == s_ObjectAllocators.end()) {
        return nullptr;
    }
    return s_ObjectAllocators.at(type.Name)();
}

Object::~Object() {
    InternalPtr::Nullify(this);
}

bool Object::IsA(const Class& type) const {
    return IsObjectOfClass(type);
}
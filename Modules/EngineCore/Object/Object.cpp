#include "Object.h"
#include "Pointer.h"

const Class Class::None = Class("");

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
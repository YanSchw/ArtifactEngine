#include "Component.h"

void Component::SetParent(Node* InParent, bool InKeepWorldTransform) {
    if (!GetParent()) {
        Super::SetParent(InParent, InKeepWorldTransform);
    } else {
        AE_ERROR("A Component may never be reparented!");
    }
}

void Component::SetRequiredParentClass(const Class& InParentClass) {
    m_ParentBaseclass = InParentClass;
}

Class Component::GetRequiredParentClass() const {
    return m_ParentBaseclass;
}
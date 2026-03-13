#include "Pointer.h"

void InternalPtr::Nullify(Object* InObject) {
    const Array<InternalPtr*>& pointers = s_PtrTable[InObject];
    for (InternalPtr* ptr : pointers) {
        ptr->m_Value = nullptr;
    }
    s_PtrTable.erase(InObject);
    s_RefCounts.erase(InObject);
}

InternalPtr::InternalPtr()
    : m_RefCounting(false)
{
}

InternalPtr::~InternalPtr() {
    if (m_Value == nullptr) {
        return;
    }

    if (InternalPtr::s_PtrTable[m_Value].Contains(this)) {
        InternalPtr::s_PtrTable[m_Value].Remove(this);
    }
}

Object* InternalPtr::GetAddress() const {
    return m_Value;
}

void InternalPtr::Assign(Object* InObject) {
    if (m_Value == InObject) {
        // No change needed...
        return;
    }

    if (m_Value) {
        if (InternalPtr::s_PtrTable[m_Value].Contains(this)) {
            InternalPtr::s_PtrTable[m_Value].Remove(this);
        }

        if (m_RefCounting) {
            InternalPtr::s_RefCounts[m_Value]--;
            if (s_RefCounts[m_Value] <= 0) {
                delete m_Value;
                if (s_RefCounts.find(m_Value) != s_RefCounts.end()) {
                    InternalPtr::Nullify(m_Value);
                }
            }
        }
    }

    m_Value = InObject;

    if (m_Value) {
        if (!InternalPtr::s_PtrTable[m_Value].Contains(this)) {
            InternalPtr::s_PtrTable[m_Value].Add(this);
        }
        if (m_RefCounting) {
            InternalPtr::s_RefCounts[m_Value]++;
        }
    }
}

Object* InternalPtr::operator=(Object* InObject) {
    Assign(InObject);
    return InObject;
}

bool InternalPtr::operator==(InternalPtr& InOther) const {
    return (m_Value == InOther.m_Value);
}


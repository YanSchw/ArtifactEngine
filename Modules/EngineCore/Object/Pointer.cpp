#include "Pointer.h"

void InternalPtr::Nullify(Object* InObject) {
    auto it = s_PtrTable.find(InObject);
    if (it == s_PtrTable.end())
        return;

    Array<InternalPtr*> pointers = it->second;
    for (InternalPtr* ptr : pointers) {
        ptr->m_Value = nullptr;
    }

    s_PtrTable.erase(InObject);
    s_RefCounts.erase(InObject);
}

InternalPtr::InternalPtr()
    : m_Value(nullptr), m_RefCounting(false)
{
}

InternalPtr::InternalPtr(const InternalPtr& InOther) {
    m_Value = nullptr;
    m_RefCounting = InOther.m_RefCounting;
    Assign(InOther.m_Value);
}

InternalPtr& InternalPtr::operator=(const InternalPtr& InOther) {
    if (this == &InOther)
        return *this;

    m_RefCounting = InOther.m_RefCounting;
    Assign(InOther.m_Value);
    return *this;
}

InternalPtr::~InternalPtr() {
    Assign(nullptr);
}

Object* InternalPtr::GetAddress() const {
    return m_Value;
}

void InternalPtr::Assign(Object* InObject) {
    if (m_Value == InObject)
        return;

    if (m_Value) {
        auto it = s_PtrTable.find(m_Value);
        if (it != s_PtrTable.end()) {
            if (it->second.Contains(this))
                it->second.Remove(this);
        }

        if (m_RefCounting) {
            auto rcIt = s_RefCounts.find(m_Value);
            if (rcIt != s_RefCounts.end()) {
                if (--rcIt->second == 0) {
                    Object* old = m_Value;
                    Nullify(old);
                    delete old;
                }
            }
        }
    }

    m_Value = InObject;

    if (m_Value) {
        if (!s_PtrTable[m_Value].Contains(this))
            s_PtrTable[m_Value].Add(this);

        if (m_RefCounting)
            s_RefCounts[m_Value]++;
    }
}

Object* InternalPtr::operator=(Object* InObject) {
    Assign(InObject);
    return InObject;
}

bool InternalPtr::operator==(InternalPtr& InOther) const {
    return m_Value == InOther.m_Value;
}
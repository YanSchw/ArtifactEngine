#pragma once
#include "Object.h"
#include "Common/Array.h"
#include <unordered_map>
#include <memory>
#include <type_traits>

template<class T, bool R> struct TObjectPtr;
class Object;

struct InternalPtr {
    static void Nullify(Object* InObject);

    InternalPtr();
    InternalPtr(const InternalPtr& InOther);
    InternalPtr& operator=(const InternalPtr& InOther);
    ~InternalPtr();

    Object* GetAddress() const;
    void Assign(Object* InObject);

    Object* operator=(Object* InObject);
    bool operator==(InternalPtr& InOther) const;

private:
    inline static std::unordered_map<Object*, Array<InternalPtr*>> s_PtrTable;
    inline static std::unordered_map<Object*, uint32_t> s_RefCounts;

    Object* m_Value = nullptr;
    bool m_RefCounting = false;

    template<typename T, bool R> friend struct TObjectPtr;
};


/** Runtime-Safe ObjectPtr implementation; automatically nullifies, if Object is deleted! */
template<typename T, bool REF_COUNTED>
struct TObjectPtr {
    TObjectPtr() {
        m_InternalPtr.m_RefCounting = REF_COUNTED;
    }

    TObjectPtr(T& InObject) {
        m_InternalPtr.m_RefCounting = REF_COUNTED;
        m_InternalPtr = (Object*)&InObject;
    }

    TObjectPtr(T* InObject) {
        m_InternalPtr.m_RefCounting = REF_COUNTED;
        m_InternalPtr = (Object*)InObject;
    }

    template<class U, bool R>
    TObjectPtr(const TObjectPtr<U, R>& InPointer) {
        static_assert(std::is_base_of<T, U>::value);
        m_InternalPtr.m_RefCounting = REF_COUNTED;
        m_InternalPtr = (Object*)InPointer.Get();
    }

    TObjectPtr(const std::shared_ptr<T>& InPointer) {
        m_InternalPtr.m_RefCounting = REF_COUNTED;
        m_InternalPtr = (Object*)InPointer.get();
    }

    ~TObjectPtr() {
        m_InternalPtr.Assign(nullptr);
    }

    inline T* Get() const {
        return (T*)m_InternalPtr.GetAddress();
    }

    inline T* operator->() const {
        return (T*)m_InternalPtr.GetAddress();
    }

    inline T& operator*() const {
        return *(T*)m_InternalPtr.GetAddress();
    }

    template<class U, bool R>
    inline bool operator==(TObjectPtr<U, R>& InOther) const {
        return m_InternalPtr.GetAddress() == InOther.m_InternalPtr.GetAddress();
    }

    template<class U>
    inline bool operator==(U*& InOther) {
        return m_InternalPtr.GetAddress() == InOther;
    }

    TObjectPtr<T, REF_COUNTED>& operator=(T& InObject) {
        m_InternalPtr = (Object*)&InObject;
        return *this;
    }

    TObjectPtr<T, REF_COUNTED>& operator=(T* InObject) {
        m_InternalPtr = (Object*)InObject;
        return *this;
    }

    template<class U, bool R>
    TObjectPtr<T, REF_COUNTED>& operator=(const TObjectPtr<U, R>& InPointer) {
        static_assert(std::is_base_of<T, U>::value);
        m_InternalPtr = InPointer.Get();
        return *this;
    }

    TObjectPtr<T, REF_COUNTED>& operator=(const std::shared_ptr<T>& InPointer) {
        m_InternalPtr = InPointer.get();
        return *this;
    }

    operator T* () const {
        return (T*)m_InternalPtr.GetAddress();
    }

    operator bool() const {
        return m_InternalPtr.GetAddress() != nullptr;
    }

protected:
    InternalPtr m_InternalPtr;

    template<class U, bool R> friend struct TObjectPtr;
    friend struct InternalPtr;
};

template<class T> using WeakObjectPtr     = TObjectPtr<T, false>;
template<class T> using SharedObjectPtr   = TObjectPtr<T, true>;


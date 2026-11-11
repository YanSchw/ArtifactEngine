#pragma once
#include "Object.h"
#include <functional>
#include <type_traits>

template<typename T>
class SubclassOf {
public:
    SubclassOf() = default;
    SubclassOf(const Class& InClass) { Set(InClass); }

    template<typename U> requires std::is_base_of_v<T, U>
    SubclassOf(const SubclassOf<U>& InOther) : m_Class(InOther.Get()) {}

    SubclassOf& operator=(const Class& InClass) {
        Set(InClass);
        return *this;
    }

    const Class& Get() const { return m_Class; }
    operator const Class&() const { return m_Class; }
    const Class* operator->() const { return &m_Class; }

    bool IsValid() const { return m_Class != Class::None; }
    explicit operator bool() const { return IsValid(); }

    static Class GetBaseClass() { return T::StaticClass(); }

    T* CreateInstance() const {
        Object* object = IsValid() ? Object::Create(m_Class) : nullptr;
        T* instance = Cast<T>(object);
        if (object && !instance) {
            delete object;
        }
        return instance;
    }

    bool operator==(const SubclassOf& InOther) const { return m_Class == InOther.m_Class; }
    bool operator==(const Class& InOther) const { return m_Class == InOther; }

private:
    void Set(const Class& InClass) {
        m_Class = InClass.IsSubclassOf(T::StaticClass()) ? InClass : Class::None;
    }

    Class m_Class;
};

template<typename T>
struct std::hash<SubclassOf<T>> {
    size_t operator()(const SubclassOf<T>& InClass) const noexcept { return std::hash<Class>()(InClass.Get()); }
};

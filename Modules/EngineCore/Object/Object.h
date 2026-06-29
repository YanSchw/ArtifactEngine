#pragma once
#include "Common/String.h"
#include <functional>
#include <string>
#include <unordered_map>

template<typename T> struct Array;

struct Class {
    std::string Name;

    Class() : Name("") {};
    Class(const Class&) = default;
    Class(const std::string& name) : Name(name) {}

    Class GetParentClass() const;
    bool IsSubclassOf(const Class& InBaseClass) const;
    static Array<Class> GetSubclassesOf(const Class& InBaseClass);

    static Array<Class> GetAllClasses();

    inline bool operator==(const Class& InOther) const { return Name == InOther.Name; }
    inline bool operator!=(const Class& InOther) const { return !operator==(InOther); }

    struct RegisterParentChildClassRelationship {
        RegisterParentChildClassRelationship(const String& InChild, const String& InParent);
    };

    const static Class None;
};

template <class To, class From> static To* Cast(From* Src);

class Object {
public:
    Object() = default;
    virtual ~Object();

    virtual bool IsObjectOfClass(const Class& type) const {
        return type.Name == "Object";
    }
    virtual Class GetClass() const {
        return Class("Object");
    }
    static Class StaticClass() {
        return Class("Object");
    }

    bool IsA(const Class& type) const;
    template<class T>
    bool IsA() const {
        return IsA(T::StaticClass());
    }
    template<class T>
    T* As() const {
        return Cast<T>(this);
    }

    static Object* Create(const Class& type);

    template<typename T>
    static T* Create(const Class& type = T::StaticClass()) {
        Object* obj = Create(type);
        return obj->As<T>();
    }

    template<typename T>
    struct RegisterArtifactClass {
        RegisterArtifactClass() {
            Object::s_ObjectAllocators[T::StaticClass().Name] = Object::InternalAllocate<T>;
        }
    };
private:
    template<typename T>
    static Object* InternalAllocate() {
        if constexpr (std::is_default_constructible<T>::value) {
            return (Object*) new T();
        } else {
            return nullptr;
        }
    }

    inline static std::unordered_map<std::string, Object*(*)(void)> s_ObjectAllocators;
    template<typename T> friend struct RegisterArtifactClass;
};

#define GENERATED_BODY_CONCAT(line) _GENERATED_BODY_##line
#define GENERATED_BODY(line) GENERATED_BODY_CONCAT(line)
#define ARTIFACT_CLASS(...) GENERATED_BODY(__LINE__)

// Dynamically cast an object type-safely.
template <class To, class From>
static To* Cast(From* Src) {
    return Src ? (Src->IsObjectOfClass(To::StaticClass()) ? (To*)Src : nullptr) : nullptr;
}

#include "Property.h"
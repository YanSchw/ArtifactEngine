#pragma once
#include "Object.h"
#include "Common/String.h"
#include "Common/Map.h"
#include "Common/Array.h"
#include "Property.gen.h"

#define PROPERTY(...)

struct Property : public Object {
    ARTIFACT_CLASS();
    String Name;
    uint64_t Offset;

    Property(const std::string& name, uint64_t offset)
        : Name(name), Offset(offset) {}

    static void RegisterTypeProperties(const String& InTypename, const Array<Property*>& InProperties) {
        s_TypeProperties[InTypename] = InProperties;
    }
    static Array<Property*> GetTypeProperties(const String& InTypeName) {
        if (!s_TypeProperties.ContainsKey(InTypeName))
            return Array<Property*>();

        return s_TypeProperties[InTypeName];
    }

private:
    inline static Map<String, Array<Property*>> s_TypeProperties;
};

struct IntProperty : public Property {
    ARTIFACT_CLASS();

    bool IsUnsigned;
    uint8_t NumBits;

    IntProperty(const std::string& name, uint64_t offset, bool isUnsigned, uint8_t numBits)
        : Property(name, offset), IsUnsigned(isUnsigned), NumBits(numBits) {}
};

struct FloatProperty : public Property {
    ARTIFACT_CLASS();

    bool IsDouble;

    FloatProperty(const std::string& name, uint64_t offset, bool isDouble)
        : Property(name, offset), IsDouble(isDouble) {}
};

struct BoolProperty : public Property {
    ARTIFACT_CLASS();

    BoolProperty(const std::string& name, uint64_t offset)
        : Property(name, offset) {}
};

struct StringProperty : public Property {
    ARTIFACT_CLASS();

    StringProperty(const std::string& name, uint64_t offset)
        : Property(name, offset) {}
};

struct SharedObjectPtrProperty : public Property {
    ARTIFACT_CLASS();

    Class InnerClass;

    SharedObjectPtrProperty(const std::string& name, uint64_t offset, const Class& innerClass)
        : Property(name, offset), InnerClass(innerClass) {}
};

struct WeakObjectPtrProperty : public Property {
    ARTIFACT_CLASS();

    Class InnerClass;

    WeakObjectPtrProperty(const std::string& name, uint64_t offset, const Class& innerClass)
        : Property(name, offset), InnerClass(innerClass) {}
};

struct StructProperty : public Property {
    ARTIFACT_CLASS();

    String InnerStructTypename;

    StructProperty(const std::string& name, uint64_t offset, const String& innerStructTypename)
        : Property(name, offset), InnerStructTypename(innerStructTypename) {}
};

struct EnumProperty : public Property {
    ARTIFACT_CLASS();

    String InnerEnumTypename;
    uint8_t ByteSize;  // size of the enum's underlying type, for raw memory access

    EnumProperty(const std::string& name, uint64_t offset, const String& innerEnumTypename, uint8_t byteSize)
        : Property(name, offset), InnerEnumTypename(innerEnumTypename), ByteSize(byteSize) {}
};

struct ArrayProperty : public Property {
    ARTIFACT_CLASS();

    using GetSizeFn = size_t(*)(void*);
    using GetElementPtrFn = void*(*)(void*, size_t);
    using AddDefaultFn = void(*)(void*);

    Property* InnerProperty;
    GetSizeFn GetSize;
    GetElementPtrFn GetElementPtr;
    AddDefaultFn AddDefault;

    ArrayProperty(const std::string& name, uint64_t offset, Property* innerProperty, GetSizeFn getSize, GetElementPtrFn getElementPtr, AddDefaultFn addDefault)
        : Property(name, offset), InnerProperty(innerProperty), GetSize(getSize), GetElementPtr(getElementPtr), AddDefault(addDefault) {}
};
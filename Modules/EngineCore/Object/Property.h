#pragma once
#include "Object.h"
#include "Common/String.h"
#include "Common/Map.h"
#include "Common/Array.h"
#include "Property.gen.h"

#define PROPERTY(...)

struct Property : public Object {
    ARTIFACT_CLASS();

    using ChangedFn = void(*)(void*);

    String Name;
    uint64_t Offset;
    ChangedFn OnChanged = nullptr;

    Property(const std::string& name, uint64_t offset)
        : Name(name), Offset(offset) {}

    void* GetValuePtr(void* InInstance) const { return (char*)InInstance + Offset; }
    void NotifyChanged(void* InInstance) const { if (OnChanged) OnChanged(InInstance); }

    Property* Changed(ChangedFn InChanged) { OnChanged = InChanged; return this; }

    void CopyValue(void* OutInstance, const void* InInstance) const;

    static void RegisterTypeProperties(const String& InTypename, const Array<Property*>& InProperties);
    static Array<Property*> GetTypeProperties(const String& InTypeName);
    static Array<Property*> GetAllTypeProperties(const Class& InClass);
    static Property* FindTypeProperty(const Class& InClass, const String& InName);
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

struct UUIDProperty : public Property {
    ARTIFACT_CLASS();

    UUIDProperty(const std::string& name, uint64_t offset)
        : Property(name, offset) {}
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
    using ClearFn = void(*)(void*);

    Property* InnerProperty;
    GetSizeFn GetSize;
    GetElementPtrFn GetElementPtr;
    AddDefaultFn AddDefault;
    ClearFn Clear;

    ArrayProperty(const std::string& name, uint64_t offset, Property* innerProperty, GetSizeFn getSize, GetElementPtrFn getElementPtr, AddDefaultFn addDefault, ClearFn clear)
        : Property(name, offset), InnerProperty(innerProperty), GetSize(getSize), GetElementPtr(getElementPtr), AddDefault(addDefault), Clear(clear) {}
};

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

struct SharedObjectPtrProperty : public Property {
    ARTIFACT_CLASS();

    Class InnerClass;

    SharedObjectPtrProperty(const std::string& name, uint64_t offset, const Class& innerClass)
        : Property(name, offset), InnerClass(innerClass) {}
};

struct ArrayProperty : public Property {
    ARTIFACT_CLASS();

    Property* InnerProperty;

    ArrayProperty(const std::string& name, uint64_t offset, Property* innerProperty)
        : Property(name, offset), InnerProperty(innerProperty) {}
};
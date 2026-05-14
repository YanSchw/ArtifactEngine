#pragma once
#include "Object.h"
#include "Common/String.h"
#include "Property.gen.h"

#define PROPERTY(...)

struct Property : public Object {
    ARTIFACT_CLASS();
    String Name;
    uint64_t Offset;

    Property(const std::string& name, uint64_t offset)
        : Name(name), Offset(offset) {}
};

struct IntProperty : public Property {
    ARTIFACT_CLASS();
    using Property::Property;
};

struct FloatProperty : public Property {
    ARTIFACT_CLASS();
    using Property::Property;
};

struct StringProperty : public Property {
    ARTIFACT_CLASS();
    using Property::Property;
};
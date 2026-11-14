#pragma once
#include "Object/Object.h"
#include "Object/Struct.h"
#include "NodeReference.gen.h"

class Node;

struct NodeReference {
    ARTIFACT_STRUCT();

    inline static const String Parent = "..";
    inline static const String Self = ".";

    NodeReference() = default;
    explicit NodeReference(const String& InPath) : Path(InPath) {}

    PROPERTY()
    String Path;

    bool IsSet() const { return !Path.empty(); }
    Node* Resolve(Node& InFrom) const;

    static NodeReference Between(Node& InFrom, Node& InTarget);
};

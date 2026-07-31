#pragma once
#include "NodeAsset.h"
#include "Blueprint.gen.h"

class Node;

class Blueprint : public NodeAsset {
public:
    ARTIFACT_CLASS();
    virtual ~Blueprint() = default;

    Node* CreateInstance() const;

    bool DependsOn(const UUID& InBlueprintId) const;
    static bool WouldRecurse(const UUID& InOwnerId, const UUID& InCandidateId);

    static Blueprint* CreateFromNode(Node& InNode, const String& InDirectory, const String& InName);
};

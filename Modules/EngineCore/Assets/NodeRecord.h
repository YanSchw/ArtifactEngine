#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "Common/UUID.h"
#include "Common/ByteString.h"
#include "Serialization/ThirdParty/nlohmann/json.hpp"
#include "NodeRecord.gen.h"

class Node;

class NodeRecord : public Object {
public:
    ARTIFACT_CLASS();

    String Name;
    String ClassName;
    bool Inherited = false;
    nlohmann::json Values = nlohmann::json::object();
    Array<SharedObjectPtr<NodeRecord>> Children;

    static SharedObjectPtr<NodeRecord> Capture(Node& InNode);

    Node* Instantiate(Node* InParent) const;
    void Apply(Node& OutNode) const;

    bool IsEmptyPatch() const;
    void CollectAssetReferences(Array<UUID>& OutIds) const;

    nlohmann::json ToJson() const;
    static SharedObjectPtr<NodeRecord> FromJson(const nlohmann::json& InJson);

    std::vector<uint8_t> ToBinary() const;
    static SharedObjectPtr<NodeRecord> FromBinary(const std::vector<uint8_t>& InBytes);
};

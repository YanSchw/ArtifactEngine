#pragma once
#include "Object/Object.h"
#include "GraphConnection.gen.h"

/** A wire between two pins, stored by node id + pin name so the document stays valid when node
 *  objects are recreated (e.g. after deserialization). From is always the output side, To the
 *  input side; NodeGraph::Connect normalizes this. */
class GraphConnection : public Object {
public:
    ARTIFACT_CLASS();

    PROPERTY()
    uint64_t FromNodeId = 0;

    PROPERTY()
    String FromPinName;

    PROPERTY()
    uint64_t ToNodeId = 0;

    PROPERTY()
    String ToPinName;

    bool TouchesNode(uint64_t InNodeId) const {
        return FromNodeId == InNodeId || ToNodeId == InNodeId;
    }
    bool UsesOutput(uint64_t InNodeId, const String& InPinName) const {
        return FromNodeId == InNodeId && FromPinName == InPinName;
    }
    bool UsesInput(uint64_t InNodeId, const String& InPinName) const {
        return ToNodeId == InNodeId && ToPinName == InPinName;
    }
};

#pragma once
#include "Object/Object.h"
#include "GraphConnection.gen.h"

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

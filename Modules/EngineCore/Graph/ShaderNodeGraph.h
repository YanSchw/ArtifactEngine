#pragma once
#include "Graph/NodeGraph.h"
#include "ShaderNodeGraph.gen.h"

class ShaderNodeGraph : public NodeGraph {
public:
    ARTIFACT_CLASS();

    virtual bool ArePinTypesCompatible(const String& InOutputType, const String& InInputType) const override;
    virtual bool AllowsNodeClass(const Class& InNodeClass) const override;
};

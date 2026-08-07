#pragma once
#include "DetailsCustomization.h"
#include "Graph/GraphNode.h"
#include "GraphNodeDetailsCustomization.gen.h"

class GraphNodeDetailsCustomization : public DetailsCustomization {
public:
    ARTIFACT_CLASS();

    virtual Class GetSupportedClass() const override { return GraphNode::StaticClass(); }
    virtual float BuildHeader(UINode& InHeader, Object* InObject, DetailsTab& InTab) override;

protected:
    virtual bool WantsClassCategory(const Class& InClass) const override;
};

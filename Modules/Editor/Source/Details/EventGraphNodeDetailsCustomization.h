#pragma once
#include "GraphNodeDetailsCustomization.h"
#include "EventGraphNodeDetailsCustomization.gen.h"

class EventGraphNodeDetailsCustomization : public GraphNodeDetailsCustomization {
public:
    ARTIFACT_CLASS();

    virtual Class GetSupportedClass() const override;
    virtual void BuildContent(UINode& InList, Object* InObject, DetailsTab& InTab) override;
};

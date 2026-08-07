#pragma once
#include "GraphNodeDetailsCustomization.h"
#include "Graph/ShaderGraphNodes.h"
#include "ShaderGraphValueDetailsCustomization.gen.h"

class ShaderGraphValueDetailsCustomization : public GraphNodeDetailsCustomization {
public:
    ARTIFACT_CLASS();

    virtual Class GetSupportedClass() const override { return ShaderGraphValueNode::StaticClass(); }
    virtual bool RebuildsOnEdit(const String& InPropertyName) const override {
        return InPropertyName == "ValueType" || InPropertyName == "ExposeAsInput";
    }

protected:
    virtual void BuildClassCategory(DetailsCategory& InCategory, const Class& InClass, Object* InObject, DetailsTab& InTab) override;
};

#pragma once
#include "NodeDetailsCustomization.h"
#include "GameFramework/AnimatorNode.h"
#include "AnimatorNodeDetailsCustomization.gen.h"

class AnimatorNodeDetailsCustomization : public NodeDetailsCustomization {
public:
    ARTIFACT_CLASS();

    virtual Class GetSupportedClass() const override { return AnimatorNode::StaticClass(); }
    virtual bool RebuildsOnEdit(const String& InPropertyName) const override {
        return InPropertyName == "m_Source" || NodeDetailsCustomization::RebuildsOnEdit(InPropertyName);
    }

protected:
    virtual void BuildClassCategory(DetailsCategory& InCategory, const Class& InClass, Object* InObject, DetailsTab& InTab) override;
};

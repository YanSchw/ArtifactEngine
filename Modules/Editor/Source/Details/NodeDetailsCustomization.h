#pragma once
#include "DetailsCustomization.h"
#include "GameFramework/Node.h"
#include "NodeDetailsCustomization.gen.h"

class Node3D;

class NodeDetailsCustomization : public DetailsCustomization {
public:
    ARTIFACT_CLASS();

    virtual Class GetSupportedClass() const override { return Node::StaticClass(); }
    virtual float BuildHeader(UINode& InHeader, Object* InObject, DetailsTab& InTab) override;

protected:
    virtual bool WantsClassCategory(const Class& InClass) const override;
    virtual void BuildClassCategory(DetailsCategory& InCategory, const Class& InClass, Object* InObject, DetailsTab& InTab) override;

private:
    static void BuildTransformCategory(DetailsCategory& InCategory, Node3D* InNode, DetailsTab& InTab);
};

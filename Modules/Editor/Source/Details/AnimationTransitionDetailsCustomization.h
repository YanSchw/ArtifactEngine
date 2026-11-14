#pragma once
#include "DetailsCustomization.h"
#include "Animation/AnimationStateMachine.h"
#include "AnimationTransitionDetailsCustomization.gen.h"

class AnimationTransitionDetailsCustomization : public DetailsCustomization {
public:
    ARTIFACT_CLASS();

    virtual Class GetSupportedClass() const override { return AnimationTransition::StaticClass(); }
    virtual bool RebuildsOnEdit(const String& InPropertyName) const override {
        return InPropertyName == "HasExitTime" || InPropertyName == "FixedDuration" || InPropertyName == "InterruptionSource";
    }
    virtual float BuildHeader(UINode& InHeader, Object* InObject, DetailsTab& InTab) override;
    virtual void BuildContent(UINode& InList, Object* InObject, DetailsTab& InTab) override;
};

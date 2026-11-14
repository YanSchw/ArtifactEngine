#include "AnimatorNodeDetailsCustomization.h"
#include "DetailsCategory.h"

void AnimatorNodeDetailsCustomization::BuildClassCategory(DetailsCategory& InCategory, const Class& InClass,
                                                          Object* InObject, DetailsTab& InTab) {
    AnimatorNode* animator = Cast<AnimatorNode>(InObject);
    if (!animator || InClass != AnimatorNode::StaticClass()) {
        NodeDetailsCustomization::BuildClassCategory(InCategory, InClass, InObject, InTab);
        return;
    }

    const String hidden = animator->GetSource() == AnimatorSource::Animation ? "m_AnimationGraph" : "m_Animation";
    const WeakObjectPtr<Object> weak(InObject);
    for (Property* property : Property::GetTypeProperties(InClass.Name)) {
        if (property->Name != hidden) {
            AddPropertyRow(*InCategory.GetBody(), InTab, weak, 0, property, 0);
        }
    }
}

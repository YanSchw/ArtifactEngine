#include "PostProcessParams.h"
#include "Object/Property.h"

static void BlendMembers(const String& InTypename, char* InOut, const char* InTarget, float InAlpha) {
    for (Property* property : Property::GetTypeProperties(InTypename)) {
        char* value = InOut + property->Offset;
        const char* target = InTarget + property->Offset;
        if (FloatProperty* floatProperty = Cast<FloatProperty>(property)) {
            if (floatProperty->IsDouble) {
                double& current = *reinterpret_cast<double*>(value);
                current += (*reinterpret_cast<const double*>(target) - current) * InAlpha;
            } else {
                float& current = *reinterpret_cast<float*>(value);
                current += (*reinterpret_cast<const float*>(target) - current) * InAlpha;
            }
        } else if (StructProperty* structProperty = Cast<StructProperty>(property)) {
            BlendMembers(structProperty->InnerStructTypename, value, target, InAlpha);
        }
    }
}

void PostProcessParams::Blend(PostProcessParams& InOut, const PostProcessParams& InTarget, float InAlpha) {
    BlendMembers("PostProcessParams", reinterpret_cast<char*>(&InOut), reinterpret_cast<const char*>(&InTarget), InAlpha);
}

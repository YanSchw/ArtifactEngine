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

PostProcessParams PostProcessParams::Neutral() {
    PostProcessParams params;
    params.AmbientOcclusion.Intensity = 0.0f;
    params.Bloom.Intensity = 0.0f;
    params.ChromaticAberration.Intensity = 0.0f;
    params.Vignette.Intensity = 0.0f;
    params.FilmGrain.Intensity = 0.0f;
    params.Tonemapping.Mode = TonemappingMode::None;
    return params;
}

static BoolProperty* FindEnabled(const StructProperty& InEffect) {
    for (Property* member : Property::GetTypeProperties(InEffect.InnerStructTypename)) {
        if (member->Name == "Enabled") {
            return Cast<BoolProperty>(member);
        }
    }
    return nullptr;
}

const Array<StructProperty*>& PostProcessParams::GetEffects() {
    static const Array<StructProperty*> s_Effects = [] {
        Array<StructProperty*> effects;
        for (Property* property : Property::GetTypeProperties("PostProcessParams")) {
            StructProperty* effect = Cast<StructProperty>(property);
            if (effect && FindEnabled(*effect)) {
                effects.Add(effect);
            }
        }
        return effects;
    }();
    return s_Effects;
}

bool PostProcessParams::IsEffectEnabled(const PostProcessParams& InParams, const StructProperty& InEffect) {
    BoolProperty* enabled = FindEnabled(InEffect);
    return enabled && *reinterpret_cast<const bool*>(reinterpret_cast<const char*>(&InParams) + InEffect.Offset + enabled->Offset);
}

void PostProcessParams::SetEffectEnabled(PostProcessParams& InOut, const StructProperty& InEffect, bool InEnabled) {
    if (BoolProperty* enabled = FindEnabled(InEffect)) {
        *reinterpret_cast<bool*>(reinterpret_cast<char*>(&InOut) + InEffect.Offset + enabled->Offset) = InEnabled;
    }
}

void PostProcessParams::BlendEffect(PostProcessParams& InOut, const PostProcessParams& InTarget, const StructProperty& InEffect, float InAlpha) {
    BlendMembers(InEffect.InnerStructTypename, reinterpret_cast<char*>(&InOut) + InEffect.Offset,
                 reinterpret_cast<const char*>(&InTarget) + InEffect.Offset, InAlpha);
}

void PostProcessParams::CopyEffect(PostProcessParams& OutParams, const PostProcessParams& InSource, const StructProperty& InEffect) {
    InEffect.CopyValue(&OutParams, &InSource);
}
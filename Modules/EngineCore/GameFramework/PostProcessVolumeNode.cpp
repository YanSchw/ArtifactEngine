#include "PostProcessVolumeNode.h"
#include "CameraNode.h"
#include "World.h"

#include <algorithm>

float PostProcessVolumeNode::GetInfluence(const Vec3& InWorldPosition) const {
    const float weight = glm::clamp(m_Weight, 0.0f, 1.0f);
    if (m_Global) {
        return weight;
    }

    const Mat4 transform = GetTransformMatrix();
    const Vec3 local = Vec3(glm::inverse(transform) * Vec4(InWorldPosition, 1.0f));
    const Vec3 extents = glm::abs(m_Extents);
    const Vec3 closest = glm::clamp(local, -extents, extents);
    if (closest == local) {
        return weight;
    }

    const float distance = glm::length(InWorldPosition - Vec3(transform * Vec4(closest, 1.0f)));
    if (m_BlendDistance <= 0.0f || distance >= m_BlendDistance) {
        return 0.0f;
    }
    return weight * (1.0f - distance / m_BlendDistance);
}

PostProcessParams PostProcessVolumeNode::Evaluate(World* InWorld, const CameraNode* InCamera) {
    PostProcessParams result = PostProcessParams::Neutral();
    if (!InWorld) {
        return result;
    }
    if (!InCamera) {
        InCamera = InWorld->GetMainCamera();
    }
    const Vec3 viewPosition = InCamera ? InCamera->GetPosition() : Vec3(0.0f);

    struct Contribution {
        const PostProcessVolumeNode* Volume;
        float Influence;
    };
    Array<Contribution> contributions;
    for (Node* node : InWorld->GetAllNodes()) {
        const PostProcessVolumeNode* volume = node->As<PostProcessVolumeNode>();
        if (!volume || !volume->IsEnabled()) {
            continue;
        }
        const float influence = volume->GetInfluence(viewPosition);
        if (influence > 0.0f) {
            contributions.Add({ volume, influence });
        }
    }
    if (contributions.IsEmpty()) {
        return result;
    }

    std::stable_sort(contributions.begin(), contributions.end(), [](const Contribution& InA, const Contribution& InB) {
        return InA.Volume->m_Layer > InB.Volume->m_Layer;
    });

    PostProcessParams blended = result;
    for (const StructProperty* effect : PostProcessParams::GetEffects()) {
        const Contribution* dominant = nullptr;
        for (const Contribution& contribution : contributions) {
            const PostProcessParams& params = contribution.Volume->m_Params;
            if (!PostProcessParams::IsEffectEnabled(params, *effect)) {
                continue;
            }
            PostProcessParams::BlendEffect(blended, params, *effect, contribution.Influence);
            if (!dominant || contribution.Influence > dominant->Influence
                || (contribution.Influence == dominant->Influence && contribution.Volume->m_Layer < dominant->Volume->m_Layer)) {
                dominant = &contribution;
            }
        }
        if (!dominant) {
            continue;
        }
        PostProcessParams::CopyEffect(result, dominant->Volume->m_Params, *effect);
        PostProcessParams::BlendEffect(result, blended, *effect, 1.0f);
    }
    return result;
}
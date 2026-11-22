#include "PreviewEnvironment.h"
#include "GameFramework/AmbientLightNode.h"
#include "GameFramework/DirectionalLightNode.h"
#include "GameFramework/SkyboxNode.h"
#include "GameFramework/World.h"

#include <algorithm>

static const Vec3 s_SunDirection = glm::normalize(Vec3(0.45f, -0.8f, 0.4f));

PreviewEnvironment::PreviewEnvironment() {
    CreateChild(SkyboxNode::StaticClass());
    CreateChild(AmbientLightNode::StaticClass());

    DirectionalLightNode* sun = CreateChild(DirectionalLightNode::StaticClass())->As<DirectionalLightNode>();
    sun->SetCastShadows(false);
    sun->SetLookDirection(s_SunDirection);
}

bool PreviewEnvironment::Contains(Node& InNode) {
    return InNode.GetRootNode()->IsA<PreviewEnvironment>();
}

void PreviewEnvironment::YieldTo(AuthoredNodes InAuthored) {
    const Array<Node*>& nodes = GetWorld()->GetAllNodes();
    for (uint32_t i = 0; i < GetChildCount(); i++) {
        Node* fallback = GetChild((int32_t)i);
        const Class fallbackClass = fallback->GetClass();
        const bool authored = std::any_of(nodes.begin(), nodes.end(), [&](Node* InNode) {
            const bool counts = InAuthored == AuthoredNodes::All || InNode->IsEnabled();
            return counts && InNode->IsA(fallbackClass) && !Contains(*InNode);
        });
        fallback->SetEnabled(!authored);
    }
}

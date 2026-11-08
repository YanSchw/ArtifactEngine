#include "SkyboxNode.h"
#include "World.h"
#include "Assets/Texture2D.h"

SkyboxNode* SkyboxNode::FindActive(World* InWorld) {
    if (!InWorld) {
        return nullptr;
    }
    for (Node* node : InWorld->GetAllNodes()) {
        SkyboxNode* sky = node->As<SkyboxNode>();
        if (sky && sky->IsEnabled()) {
            return sky;
        }
    }
    return nullptr;
}

#include "GizmoLayer.h"
#include "Tabs/MajorTab.h"
#include "UI/EditorStyle.h"
#include "Assets/AssetManager.h"
#include "Assets/Mesh.h"
#include "Common/UUID.h"
#include "GameFramework/World.h"
#include "GameFramework/CameraNode.h"
#include "GameFramework/Node3D.h"
#include <algorithm>

static const Vec4 s_CameraColor = HexColor(0xB9C2CC);
static const Vec4 s_CameraSelectedColor = HexColor(0x26BBFF);
static constexpr float s_ScreenScale = 0.13f;
static constexpr float s_MinScale = 0.12f;
static constexpr float s_MaxScale = 8.0f;

GizmoLayer::GizmoLayer() {
    m_CameraMesh = AssetManager::Get().GetAsset<Mesh>(UUID::FromString("b1c2d3e4-0007-4a00-9000-000000000007"));
}

float GizmoLayer::ScaleForDistance(const Vec3& InGizmoPos, CameraNode* InViewCamera) const {
    if (!InViewCamera) {
        return 1.0f;
    }
    const float distance = glm::length(InGizmoPos - InViewCamera->GetPosition());
    return std::clamp(distance * s_ScreenScale, s_MinScale, s_MaxScale);
}

void GizmoLayer::Collect(World* InWorld, CameraNode* InViewCamera, MajorTab* InMajorTab, Array<GizmoDraw>& OutGizmos) {
    Mesh* cameraMesh = m_CameraMesh.Get();
    if (!InWorld || !cameraMesh) {
        return;
    }

    for (Node* node : InWorld->GetAllNodes()) {
        CameraNode* camera = node->As<CameraNode>();
        if (!camera || camera == InViewCamera || !camera->IsEnabled()) {
            continue;
        }

        const Vec3 position = camera->GetPosition();
        GizmoDraw draw;
        draw.MeshPtr = cameraMesh;
        draw.Transform = Node3D::CalculateTransformMatrix(position, camera->GetRotation(), Vec3(1.0));
        draw.Color = (InMajorTab && InMajorTab->IsSelected(camera)) ? s_CameraSelectedColor : s_CameraColor;
        draw.NodeId = camera->GetNodeId();
        OutGizmos.Add(draw);
    }
}

#include "GizmoLayer.h"
#include "Tabs/MajorTab.h"
#include "Settings/EditorSettings.h"
#include "UI/EditorStyle.h"
#include "Assets/AssetManager.h"
#include "Assets/Mesh.h"
#include "Common/UUID.h"
#include "GameFramework/World.h"
#include "GameFramework/CameraNode.h"
#include "GameFramework/DebugDraw.h"
#include "GameFramework/DirectionalLightNode.h"
#include "GameFramework/PointLightNode.h"
#include "GameFramework/AudioSourceNode.h"
#include "GameFramework/AudioListenerNode.h"
#include "GameFramework/Node3D.h"
#include "GameFramework/PostProcessVolumeNode.h"
#include "GameFramework/ParticleSystemEmitterNode.h"
#include "Particles/ParticlePreview.h"
#include <algorithm>

static const Vec4 s_CameraColor = HexColor(0xB9C2CC);
static const Vec4 s_CameraSelectedColor = HexColor(0x26BBFF);
static const Vec4 s_ColliderColor = HexColor(0xE0566E);
static const Vec4 s_TriggerColor = HexColor(0x4ADE80);
static const Vec4 s_CharacterColor = HexColor(0xD98FA8);
static const Vec4 s_SunColor = HexColor(0xFFD98A);
static const Vec4 s_VolumeColor = HexColor(0xB48CF2);
static const Vec4 s_AudioColor = HexColor(0x5EEAD4);
static const Vec4 s_ListenerColor = HexColor(0x2DD4BF);
static constexpr float s_RangeAlpha = 0.45f;
static constexpr float s_MarkerSize = 0.35f;
static constexpr float s_ScreenScale = 0.13f;
static constexpr float s_MinScale = 0.12f;
static constexpr float s_MaxScale = 8.0f;
static constexpr float s_GizmoThickness = 1.5f;
static constexpr float s_FrustumLength = 6.0f;
static constexpr float s_SunArrowLength = 1.6f;
static constexpr float s_SunArrowHeadLength = 0.4f;
static constexpr float s_SunArrowHeadRadius = 0.16f;

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

void GizmoLayer::DrawShapeOutline(World* InWorld, Node3D* InNode) const {
    bool isTrigger = false;
    InNode->GetPropertyValue("m_IsTrigger", isTrigger);
    const Vec4 color = isTrigger ? s_TriggerColor : s_ColliderColor;

    const Vec3 position = InNode->GetPosition();
    const Quat rotation = InNode->GetRotation();
    const Vec3 scale = glm::abs(InNode->GetScale());
    const float radialScale = glm::max(scale.x, scale.z);

    const Class nodeClass = InNode->GetClass();
    Vec3 halfExtents;
    float radius = 0.0f;
    float halfHeight = 0.0f;

    if (nodeClass.IsSubclassOf(Class("BoxShapeNode")) && InNode->GetPropertyValue("m_HalfExtents", halfExtents)) {
        DebugDraw::Box(InWorld, position, halfExtents * scale, rotation, color, s_GizmoThickness);
    } else if (nodeClass.IsSubclassOf(Class("SphereShapeNode")) && InNode->GetPropertyValue("m_Radius", radius)) {
        DebugDraw::Sphere(InWorld, position, radius * glm::max(scale.x, glm::max(scale.y, scale.z)), color, s_GizmoThickness);
    } else if (nodeClass.IsSubclassOf(Class("CapsuleShapeNode"))
               && InNode->GetPropertyValue("m_Radius", radius) && InNode->GetPropertyValue("m_HalfHeight", halfHeight)) {
        DebugDraw::Capsule(InWorld, position, halfHeight * scale.y, radius * radialScale, rotation, color, s_GizmoThickness);
    } else if (nodeClass.IsSubclassOf(Class("CylinderShapeNode"))
               && InNode->GetPropertyValue("m_Radius", radius) && InNode->GetPropertyValue("m_HalfHeight", halfHeight)) {
        DebugDraw::Cylinder(InWorld, position, halfHeight * scale.y, radius * radialScale, rotation, color, s_GizmoThickness);
    }
}

void GizmoLayer::DrawCharacterOutline(World* InWorld, Node3D* InNode) const {
    float radius = 0.0f;
    float height = 0.0f;
    if (!InNode->GetPropertyValue("m_Radius", radius) || !InNode->GetPropertyValue("m_Height", height)) {
        return;
    }

    const float cylinderHalfHeight = glm::max(height * 0.5f - radius, 0.01f);
    DebugDraw::Capsule(InWorld, InNode->GetPosition(), cylinderHalfHeight, radius,
                       InNode->GetRotation(), s_CharacterColor, s_GizmoThickness);
}

void GizmoLayer::DrawCameraFrustum(World* InWorld, CameraNode* InCamera, const Vec4& InColor) const {
    const bool perspective = InCamera->GetProjectionType() == CameraNode::ProjectionType::Perspective;
    const float halfHeight = perspective
        ? std::tan(glm::radians(InCamera->GetPerspectiveVerticalFOV()) * 0.5f) * s_FrustumLength
        : InCamera->GetOrthographicSize() * 0.5f;
    const float halfWidth = halfHeight * glm::max(InCamera->GetAspectRatio(), 0.01f);

    const Vec3 origin = InCamera->GetPosition();
    const Vec3 right = InCamera->GetRightVector();
    const Vec3 up = InCamera->GetUpVector();
    const Vec3 farCenter = origin + InCamera->GetForwardVector() * s_FrustumLength;

    Vec3 nearCorners[4];
    Vec3 farCorners[4];
    for (int32_t corner = 0; corner < 4; corner++) {
        const float x = (corner == 0 || corner == 3) ? -1.0f : 1.0f;
        const float y = (corner < 2) ? -1.0f : 1.0f;
        nearCorners[corner] = perspective ? origin : origin + right * (halfWidth * x) + up * (halfHeight * y);
        farCorners[corner] = farCenter + right * (halfWidth * x) + up * (halfHeight * y);
    }

    for (int32_t corner = 0; corner < 4; corner++) {
        const int32_t next = (corner + 1) % 4;
        DebugDraw::Line(InWorld, farCorners[corner], farCorners[next], InColor, s_GizmoThickness);
        DebugDraw::Line(InWorld, nearCorners[corner], nearCorners[next], InColor, s_GizmoThickness);
        DebugDraw::Line(InWorld, nearCorners[corner], farCorners[corner], InColor, s_GizmoThickness);
    }
}

void GizmoLayer::DrawSunDirection(World* InWorld, DirectionalLightNode* InLight) const {
    const Vec3 direction = InLight->GetDirection();
    const Vec3 origin = InLight->GetPosition();
    const Vec3 tip = origin + direction * s_SunArrowLength;
    const Vec3 right = InLight->GetRightVector();
    const Vec3 up = InLight->GetUpVector();

    DebugDraw::Line(InWorld, origin, tip, s_SunColor, s_GizmoThickness);

    const Vec3 headBase = tip - direction * s_SunArrowHeadLength;
    for (int32_t corner = 0; corner < 4; corner++) {
        const Vec3 offset = (corner < 2 ? right : up) * (corner % 2 == 0 ? s_SunArrowHeadRadius : -s_SunArrowHeadRadius);
        DebugDraw::Line(InWorld, tip, headBase + offset, s_SunColor, s_GizmoThickness);
    }
}

static Vec4 WithAlpha(const Vec4& InColor, float InAlpha) {
    return Vec4(Vec3(InColor), InAlpha);
}

void GizmoLayer::DrawMarker(World* InWorld, const Vec3& InCenter, float InSize, const Vec4& InColor) const {
    const Vec3 tips[6] = { Vec3(InSize, 0, 0), Vec3(-InSize, 0, 0), Vec3(0, InSize, 0),
                           Vec3(0, -InSize, 0), Vec3(0, 0, InSize), Vec3(0, 0, -InSize) };
    for (int32_t a = 0; a < 6; a++) {
        for (int32_t b = a + 1; b < 6; b++) {
            if (a / 2 != b / 2) {
                DebugDraw::Line(InWorld, InCenter + tips[a], InCenter + tips[b], InColor, s_GizmoThickness);
            }
        }
    }
}

void GizmoLayer::DrawPointLight(World* InWorld, PointLightNode* InLight, float InMarkerScale, bool InSelected) const {
    const Vec4 lightColor = WithAlpha(InLight->GetColor(), 1.0f);
    DrawMarker(InWorld, InLight->GetPosition(), s_MarkerSize * InMarkerScale, InSelected ? s_CameraSelectedColor : lightColor);
    if (InSelected) {
        DebugDraw::Sphere(InWorld, InLight->GetPosition(), InLight->GetRadius(), WithAlpha(lightColor, s_RangeAlpha), s_GizmoThickness);
    }
}

void GizmoLayer::DrawAudioSource(World* InWorld, AudioSourceNode* InSource, float InMarkerScale, bool InSelected) const {
    const Vec4 color = InSelected ? s_CameraSelectedColor : s_AudioColor;
    DrawMarker(InWorld, InSource->GetPosition(), s_MarkerSize * InMarkerScale, color);
    if (!InSelected || !InSource->IsSpatial()) {
        return;
    }
    const AudioAttenuation& attenuation = InSource->GetAttenuation();
    DebugDraw::Sphere(InWorld, InSource->GetPosition(), attenuation.MinDistance, WithAlpha(s_AudioColor, 0.8f), s_GizmoThickness);
    DebugDraw::Sphere(InWorld, InSource->GetPosition(), attenuation.MaxDistance, WithAlpha(s_AudioColor, s_RangeAlpha), s_GizmoThickness);
}

void GizmoLayer::DrawAudioListener(World* InWorld, AudioListenerNode* InListener, float InMarkerScale) const {
    const float size = s_MarkerSize * InMarkerScale;
    const Vec3 position = InListener->GetPosition();
    DrawMarker(InWorld, position, size, s_ListenerColor);
    DebugDraw::Line(InWorld, position, position + InListener->GetForwardVector() * size * 3.0f, s_ListenerColor, s_GizmoThickness);
}

void GizmoLayer::CollectNode(World* InWorld, Node3D* InNode, CameraNode* InViewCamera, bool InSelected,
                             const EditorGizmoSettings& InSettings, Array<GizmoDraw>& OutGizmos) const {
    const Class nodeClass = InNode->GetClass();

    if (nodeClass.IsSubclassOf(Class("ShapeNode"))) {
        if (InSettings.Colliders) {
            DrawShapeOutline(InWorld, InNode);
        }
    } else if (nodeClass.IsSubclassOf(Class("CharacterNode"))) {
        if (InSettings.Characters) {
            DrawCharacterOutline(InWorld, InNode);
        }
    }

    if (DirectionalLightNode* light = InNode->As<DirectionalLightNode>()) {
        if (InSettings.DirectionalLights) {
            DrawSunDirection(InWorld, light);
        }
    } else if (PointLightNode* pointLight = InNode->As<PointLightNode>()) {
        if (InSettings.PointLights) {
            DrawPointLight(InWorld, pointLight, ScaleForDistance(pointLight->GetPosition(), InViewCamera), InSelected);
        }
    } else if (AudioSourceNode* source = InNode->As<AudioSourceNode>()) {
        if (InSettings.AudioSources) {
            DrawAudioSource(InWorld, source, ScaleForDistance(source->GetPosition(), InViewCamera), InSelected);
        }
    } else if (AudioListenerNode* listener = InNode->As<AudioListenerNode>()) {
        if (InSettings.AudioListeners) {
            DrawAudioListener(InWorld, listener, ScaleForDistance(listener->GetPosition(), InViewCamera));
        }
    } else if (PostProcessVolumeNode* volume = InNode->As<PostProcessVolumeNode>()) {
        if (InSettings.PostProcessVolumes && !volume->IsGlobal()) {
            DebugDraw::Box(InWorld, volume->GetPosition(), glm::abs(volume->GetExtents() * volume->GetScale()),
                           volume->GetRotation(), InSelected ? s_CameraSelectedColor : s_VolumeColor, s_GizmoThickness);
        }
    } else if (ParticleSystemEmitterNode* emitter = InNode->As<ParticleSystemEmitterNode>()) {
        if (InSelected && emitter->GetSystem()) {
            ParticlePreview::DrawShapes(*InWorld, *emitter->GetSystem(), emitter->GetTransformMatrix());
        }
    }

    CameraNode* camera = InNode->As<CameraNode>();
    if (!camera || camera == InViewCamera || !InSettings.Cameras) {
        return;
    }

    const Vec4 cameraColor = InSelected ? s_CameraSelectedColor : s_CameraColor;
    DrawCameraFrustum(InWorld, camera, cameraColor);

    if (Mesh* cameraMesh = m_CameraMesh.Get()) {
        GizmoDraw draw;
        draw.MeshPtr = cameraMesh;
        draw.Transform = Node3D::CalculateTransformMatrix(camera->GetPosition(), camera->GetRotation(), Vec3(1.0));
        draw.Color = cameraColor;
        draw.NodeId = camera->GetNodeId();
        OutGizmos.Add(draw);
    }
}

void GizmoLayer::Collect(World* InWorld, CameraNode* InViewCamera, MajorTab* InMajorTab, Array<GizmoDraw>& OutGizmos) {
    const EditorGizmoSettings& settings = EditorSettings::Get().Gizmos;
    if (!InWorld || !settings.ShowGizmos) {
        return;
    }

    for (Node* node : InWorld->GetAllNodes()) {
        Node3D* node3D = node->As<Node3D>();
        if (node3D && node3D->IsEnabled()) {
            CollectNode(InWorld, node3D, InViewCamera, InMajorTab && InMajorTab->IsSelected(node3D), settings, OutGizmos);
        }
    }
}

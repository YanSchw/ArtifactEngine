#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "GizmoRenderer.h"
#include "GizmoLayer.gen.h"

class World;
class MajorTab;
class Mesh;
class CameraNode;
class DirectionalLightNode;
class PointLightNode;
class AudioSourceNode;
class AudioListenerNode;
class Node3D;
struct EditorGizmoSettings;

/** Decides what the editor overlays on a viewport. */
class GizmoLayer : public Object {
public:
    ARTIFACT_CLASS();

    GizmoLayer();

    void Collect(World* InWorld, CameraNode* InViewCamera, MajorTab* InMajorTab, Array<GizmoDraw>& OutGizmos);

private:
    /** Gizmos hold a constant on-screen size, so they stay usable at any distance. */
    float ScaleForDistance(const Vec3& InGizmoPos, CameraNode* InViewCamera) const;

    void DrawShapeOutline(World* InWorld, Node3D* InNode) const;
    void DrawCharacterOutline(World* InWorld, Node3D* InNode) const;
    void DrawCameraFrustum(World* InWorld, CameraNode* InCamera, const Vec4& InColor) const;
    void DrawSunDirection(World* InWorld, DirectionalLightNode* InLight) const;
    void DrawPointLight(World* InWorld, PointLightNode* InLight, float InMarkerScale, bool InSelected) const;
    void DrawAudioSource(World* InWorld, AudioSourceNode* InSource, float InMarkerScale, bool InSelected) const;
    void DrawAudioListener(World* InWorld, AudioListenerNode* InListener, float InMarkerScale) const;
    void DrawMarker(World* InWorld, const Vec3& InCenter, float InSize, const Vec4& InColor) const;
    void CollectNode(World* InWorld, Node3D* InNode, CameraNode* InViewCamera, bool InSelected,
                     const EditorGizmoSettings& InSettings, Array<GizmoDraw>& OutGizmos) const;

    WeakObjectPtr<Mesh> m_CameraMesh;
};

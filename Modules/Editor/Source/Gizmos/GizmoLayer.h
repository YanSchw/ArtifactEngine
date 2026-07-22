#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "GizmoRenderer.h"
#include "GizmoLayer.gen.h"

class World;
class MajorTab;
class Mesh;
class CameraNode;

/** Decides what the editor overlays on a viewport. */
class GizmoLayer : public Object {
public:
    ARTIFACT_CLASS();

    GizmoLayer();

    void Collect(World* InWorld, CameraNode* InViewCamera, MajorTab* InMajorTab, Array<GizmoDraw>& OutGizmos);

private:
    /** Gizmos hold a constant on-screen size, so they stay usable at any distance. */
    float ScaleForDistance(const Vec3& InGizmoPos, CameraNode* InViewCamera) const;

    WeakObjectPtr<Mesh> m_CameraMesh;
};

#pragma once
#include "Node3D.h"
#include "PostProcessParams.h"
#include "PostProcessVolumeNode.gen.h"

class CameraNode;
class World;

/** Configures the PostProcess stage, either everywhere (global) or inside a box around this node.
 *  Overlapping volumes are layered and blended by Evaluate, so render passes never do it themselves. */
class PostProcessVolumeNode : public Node3D {
public:
    ARTIFACT_CLASS();

    /** The params every pass of a frame should use: all enabled volumes of InWorld, blended for where
     *  InCamera stands (the world's main camera when null). Volumes are applied from the highest
     *  layer down, so lower layers end on top; members that cannot be blended come from the volume
     *  with the greatest influence, the lowest layer breaking ties. Defaults when nothing applies. */
    static PostProcessParams Evaluate(World* InWorld, const CameraNode* InCamera = nullptr);

    /** How strongly this volume applies at InWorldPosition, from 0 up to its weight. */
    float GetInfluence(const Vec3& InWorldPosition) const;

    bool IsGlobal() const { return m_Global; }
    void SetGlobal(bool InGlobal) { m_Global = InGlobal; }

    Vec3 GetExtents() const { return m_Extents; }
    void SetExtents(const Vec3& InExtents) { m_Extents = InExtents; }

    float GetBlendDistance() const { return m_BlendDistance; }
    void SetBlendDistance(float InDistance) { m_BlendDistance = InDistance; }

    float GetWeight() const { return m_Weight; }
    void SetWeight(float InWeight) { m_Weight = InWeight; }

    int32_t GetLayer() const { return m_Layer; }
    void SetLayer(int32_t InLayer) { m_Layer = InLayer; }

    const PostProcessParams& GetParams() const { return m_Params; }
    PostProcessParams& GetParams() { return m_Params; }

private:
    PROPERTY()
    bool m_Global = true;

    /** Half the size of the box in local space. Unused while global. */
    PROPERTY()
    Vec3 m_Extents = Vec3(5.0f);

    /** Distance outside the box over which the volume fades out. */
    PROPERTY()
    float m_BlendDistance = 2.0f;

    PROPERTY()
    float m_Weight = 1.0f;

    /** Lower layers are applied on top of higher ones. */
    PROPERTY()
    int32_t m_Layer = 0;

    PROPERTY()
    PostProcessParams m_Params;
};

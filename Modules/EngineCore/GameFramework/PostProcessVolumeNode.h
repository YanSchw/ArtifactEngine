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
     *  InCamera stands (the world's main camera when null). Every effect is blended separately across
     *  only the volumes that enable it, from the highest layer down, so lower layers end on top;
     *  members that cannot be blended come from the most influential of those volumes, the lowest layer
     *  breaking ties. An effect no volume enables stays as PostProcessParams::Neutral() has it. */
    static PostProcessParams Evaluate(World* InWorld, const CameraNode* InCamera = nullptr);

    /** How strongly this volume applies at InWorldPosition, from 0 up to its weight. */
    FUNCTION(Pure)
    float GetInfluence(const Vec3& InWorldPosition) const;

    FUNCTION(Pure)
    bool IsGlobal() const { return m_Global; }
    FUNCTION()
    void SetGlobal(bool InGlobal) { m_Global = InGlobal; }

    FUNCTION(Pure)
    Vec3 GetExtents() const { return m_Extents; }
    FUNCTION()
    void SetExtents(const Vec3& InExtents) { m_Extents = InExtents; }

    FUNCTION(Pure)
    float GetBlendDistance() const { return m_BlendDistance; }
    FUNCTION()
    void SetBlendDistance(float InDistance) { m_BlendDistance = InDistance; }

    FUNCTION(Pure)
    float GetWeight() const { return m_Weight; }
    FUNCTION()
    void SetWeight(float InWeight) { m_Weight = InWeight; }

    FUNCTION(Pure)
    int32_t GetLayer() const { return m_Layer; }
    FUNCTION()
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

    PROPERTY(Range = (0, 1))
    float m_Weight = 1.0f;

    /** Lower layers are applied on top of higher ones. */
    PROPERTY()
    int32_t m_Layer = 0;

    PROPERTY()
    PostProcessParams m_Params;
};

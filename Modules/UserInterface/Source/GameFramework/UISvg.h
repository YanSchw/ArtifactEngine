#pragma once
#include "UINode.h"
#include "Object/Pointer.h"
#include "Assets/VectorImage.h"
#include "UISvg.gen.h"

/** Draws a VectorImage (SVG) asset into the node's rect. */
class UISvg : public UINode {
public:
    ARTIFACT_CLASS();

    WeakObjectPtr<VectorImage> Image;
    Vec4 Tint = Vec4(1.0f);
    bool PreserveAspect = true;

    virtual void Paint(UIDrawList& OutDrawList) override;

private:
    SvgMesh m_Mesh;
    VectorImage* m_CachedImage = nullptr;
    float m_CachedScale = 0.0f;
};

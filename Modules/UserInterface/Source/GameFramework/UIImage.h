#pragma once
#include "UINode.h"
#include "Object/Pointer.h"
#include "UIImage.gen.h"

class Texture;

/** How a UIImage reveals itself as FillAmount goes 0 -> 1. */
enum class UIImageFill : uint8_t {
    None = 0,        // always the full rect
    Horizontal = 1,  // reveals left -> right
    Vertical = 2,    // reveals bottom -> top
    Radial = 3       // pie sweep around the center, starting at FillOriginDegrees
};

class UIImage : public UINode {
public:
    ARTIFACT_CLASS();

    SharedObjectPtr<Texture> Image;
    Vec4 Tint = Vec4(1.0f);
    UIImageFill FillMethod = UIImageFill::None;
    float FillAmount = 1.0f;
    bool FillClockwise = true;
    float FillOriginDegrees = 0.0f;

    virtual void Paint(UIDrawList& OutDrawList) override;
};

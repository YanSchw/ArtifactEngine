#pragma once
#include "UINode.h"
#include "Object/Pointer.h"
#include "Object/Enum.h"
#include "UIImage.gen.h"

class Texture;

/** How a UIImage reveals itself as FillAmount goes 0 -> 1. */
ARTIFACT_ENUM();
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
    PROPERTY()
    Color Tint = Color(1.0f);
    PROPERTY()
    UIImageFill FillMethod = UIImageFill::None;
    PROPERTY(Range = (0, 1))
    float FillAmount = 1.0f;
    PROPERTY()
    bool FillClockwise = true;
    PROPERTY(Range = (0, 360))
    float FillOriginDegrees = 0.0f;

    virtual void Paint(UIDrawList& OutDrawList) override;
};

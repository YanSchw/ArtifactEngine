#pragma once
#include "UINode.h"
#include "UILabel.gen.h"

class Font;

enum class UIHAlign : uint8_t { Left, Center, Right };
enum class UIVAlign : uint8_t { Top, Middle, Bottom };

/** A UINode that draws a single line of SDF text, aligned within its content rect. Set the public
 *  fields directly; FontAsset falls back to UINode::GetDefaultFont() when null. */
class UILabel : public UINode {
public:
    ARTIFACT_CLASS();

    String Text;
    Font* FontAsset = nullptr;
    float FontSize = 18.0f;
    Vec4 Color = Vec4(1.0f);
    UIHAlign HAlign = UIHAlign::Left;
    UIVAlign VAlign = UIVAlign::Top;

    virtual void Paint(UIDrawList& OutDrawList) override;
};

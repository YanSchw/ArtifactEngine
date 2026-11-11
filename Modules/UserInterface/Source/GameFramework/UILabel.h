#pragma once
#include "UINode.h"
#include "Object/Pointer.h"
#include "UILabel.gen.h"

class Font;

ARTIFACT_ENUM();
enum class UIHAlign : uint8_t { Left, Center, Right };

ARTIFACT_ENUM();
enum class UIVAlign : uint8_t { Top, Middle, Bottom };

/** A UINode that draws a single line of SDF text, aligned within its content rect. Set the public
 *  fields directly; FontAsset falls back to UINode::GetDefaultFont() when null. */
class UILabel : public UINode {
public:
    ARTIFACT_CLASS();

    PROPERTY()
    String Text;

    FUNCTION(Pure)
    String GetText() const { return Text; }
    FUNCTION()
    void SetText(const String& InText) { Text = InText; }

    PROPERTY()
    WeakObjectPtr<Font> FontAsset;

    PROPERTY()
    float FontSize = 18.0f;

    PROPERTY()
    Vec4 Color = Vec4(1.0f);

    PROPERTY()
    UIHAlign HAlign = UIHAlign::Left;

    PROPERTY()
    UIVAlign VAlign = UIVAlign::Top;

    virtual void Paint(UIDrawList& OutDrawList) override;
};

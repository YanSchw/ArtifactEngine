#pragma once
#include "UINode.h"
#include "Object/Pointer.h"
#include <functional>
#include "UIButton.gen.h"

class UILabel;

/** A clickable UINode. Paints a state-tinted background and hosts a centered caption label.
 *  Set the colors / OnClick directly and call SetCaption() for the text. OnClick fires when a
 *  press both starts and ends inside the button. */
class UIButton : public UINode {
public:
    ARTIFACT_CLASS();

    UIButton();

    Vec4 NormalColor = Vec4(0.20f, 0.22f, 0.26f, 1.0f);
    Vec4 HoverColor = Vec4(0.28f, 0.32f, 0.38f, 1.0f);
    Vec4 PressedColor = Vec4(0.14f, 0.16f, 0.20f, 1.0f);
    std::function<void()> OnClick;

    void SetCaption(const String& InText);

    bool IsHovered() const { return m_Hovered; }
    bool IsPressed() const { return m_Pressed; }

    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;

private:
    WeakObjectPtr<UILabel> m_Label;
    bool m_Hovered = false;
    bool m_Pressed = false;
};

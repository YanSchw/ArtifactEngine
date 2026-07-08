#pragma once
#include "UINode.h"
#include "Object/Pointer.h"
#include <functional>
#include "UIButton.gen.h"

class UILabel;

class UIButton : public UINode {
public:
    ARTIFACT_CLASS();

    UIButton();

    Vec4 NormalColor = Vec4(0.20f, 0.22f, 0.26f, 1.0f);
    Vec4 HoverColor = Vec4(0.28f, 0.32f, 0.38f, 1.0f);
    Vec4 PressedColor = Vec4(0.14f, 0.16f, 0.20f, 1.0f);
    std::function<void()> Clicked;

    void SetCaption(const String& InText);

    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnClick() override { if (Clicked) Clicked(); }

private:
    WeakObjectPtr<UILabel> m_Label;
};

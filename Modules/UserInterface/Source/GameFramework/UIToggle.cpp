#include "UIToggle.h"

UIToggle::UIToggle() {
    Interactable = true;
    Cursor = CursorIcon::Hand;
    Size = Vec2(20.0f);
}

void UIToggle::OnClick() {
    IsOn = !IsOn;
    if (Changed) {
        Changed(IsOn);
    }
}

void UIToggle::OnUIUpdate(const UIFrameContext& InContext) {
    (void)InContext;
    if (Graphic) {
        Graphic->SetEnabled(IsOn);
    }
}

#include "UIVirtualGamepad.h"
#include "Assets/Font.h"
#include "Rendering/UIDrawList.h"
#include "InputSystem/InputSystem.h"
#include "InputSystem/MultiTouchDevice.h"

#include <numbers>

namespace {
    constexpr Vec2 s_DesignSize = Vec2(2000.0f, 1000.0f);
    constexpr float s_LeftColumnEnd = 700.0f;
    constexpr float s_RightColumnStart = 1300.0f;
    constexpr float s_TopRowEnd = 300.0f;
    constexpr float s_ButtonHitScale = 1.15f;
    constexpr int32_t s_Segments = 48;

    enum class Shape : uint8_t { Circle, Pill };
    enum class Icon : uint8_t { Label, ArrowLeft, ArrowRight, Controller };
    enum class Output : uint8_t { Button, LeftTrigger, RightTrigger };

    struct ButtonSpec {
        Vec2 Center;
        Vec2 Extent;
        Shape Kind;
        Icon Glyph;
        const char* Label;
        Output Target;
        GamepadCode Code;
    };

    constexpr float s_ButtonRadius = 73.0f;
    constexpr float s_FaceLabelHeight = 58.0f;
    constexpr float s_ShoulderLabelHeight = 46.0f;

    const ButtonSpec s_Buttons[] = {
        { { 117.0f, 220.0f }, Vec2(s_ButtonRadius), Shape::Circle, Icon::Label, "LT", Output::LeftTrigger, GamepadCode::South },
        { { 396.0f, 220.0f }, Vec2(s_ButtonRadius), Shape::Circle, Icon::Label, "LB", Output::Button, GamepadCode::LeftBumper },
        { { 1602.0f, 220.0f }, Vec2(s_ButtonRadius), Shape::Circle, Icon::Label, "RB", Output::Button, GamepadCode::RightBumper },
        { { 1881.0f, 220.0f }, Vec2(s_ButtonRadius), Shape::Circle, Icon::Label, "RT", Output::RightTrigger, GamepadCode::South },
        { { 814.0f, 110.0f }, Vec2(73.0f, 36.0f), Shape::Pill, Icon::ArrowLeft, "", Output::Button, GamepadCode::Back },
        { { 1000.0f, 110.0f }, Vec2(55.0f), Shape::Circle, Icon::Controller, "", Output::Button, GamepadCode::Guide },
        { { 1185.0f, 110.0f }, Vec2(73.0f, 36.0f), Shape::Pill, Icon::ArrowRight, "", Output::Button, GamepadCode::Start },
        { { 1741.0f, 397.0f }, Vec2(s_ButtonRadius), Shape::Circle, Icon::Label, "Y", Output::Button, GamepadCode::North },
        { { 1602.0f, 537.0f }, Vec2(s_ButtonRadius), Shape::Circle, Icon::Label, "X", Output::Button, GamepadCode::West },
        { { 1881.0f, 537.0f }, Vec2(s_ButtonRadius), Shape::Circle, Icon::Label, "B", Output::Button, GamepadCode::East },
        { { 1741.0f, 677.0f }, Vec2(s_ButtonRadius), Shape::Circle, Icon::Label, "A", Output::Button, GamepadCode::South },
        { { 117.0f, 880.0f }, Vec2(s_ButtonRadius), Shape::Circle, Icon::Label, "L3", Output::Button, GamepadCode::LeftThumb },
        { { 1881.0f, 880.0f }, Vec2(s_ButtonRadius), Shape::Circle, Icon::Label, "R3", Output::Button, GamepadCode::RightThumb },
    };
    constexpr int32_t s_ButtonCount = (int32_t)(sizeof(s_Buttons) / sizeof(s_Buttons[0]));

    constexpr Vec2 s_DPadCenter = Vec2(257.0f, 537.0f);
    constexpr float s_DPadSpacing = 140.0f;
    constexpr float s_DPadRegion = 215.0f;
    constexpr float s_DPadDeadZone = 0.2f;

    struct DPadSpec {
        Vec2 Direction;
        GamepadCode Code;
    };

    const DPadSpec s_DPad[] = {
        { { 0.0f, -1.0f }, GamepadCode::DPadUp },
        { { 1.0f, 0.0f }, GamepadCode::DPadRight },
        { { 0.0f, 1.0f }, GamepadCode::DPadDown },
        { { -1.0f, 0.0f }, GamepadCode::DPadLeft },
    };

    constexpr Vec2 s_LeftStickCenter = Vec2(521.0f, 802.0f);
    constexpr Vec2 s_RightStickCenter = Vec2(1478.0f, 802.0f);
    constexpr float s_StickOuterRadius = 162.0f;
    constexpr float s_StickInnerRadius = 110.0f;
    constexpr float s_StickKnobRadius = 67.0f;
    constexpr float s_StickTravel = s_StickOuterRadius - s_StickKnobRadius;

    constexpr float s_OffscreenCursor = -1.0e5f;
}

UIVirtualGamepad::UIVirtualGamepad() {
    Fill();
    Padding = UIPadding(UIValue::Rel(0.05f), UIValue::Rel(0.04f));
}

void UIVirtualGamepad::BeginPlay() {
    Super::BeginPlay();
    VirtualGamepadDevice* device = new VirtualGamepadDevice();
    InputSystem::Get().AddDevice(device);
    m_Device = device;
}

void UIVirtualGamepad::EndPlay() {
    if (VirtualGamepadDevice* device = m_Device.Get()) {
        InputSystem::Get().RemoveDevice(device);
    }
    m_Device = nullptr;
    Super::EndPlay();
}

bool UIVirtualGamepad::IsVisible() const {
    return !HideWithoutTouchscreen || MultiTouchDevice::Instance() != nullptr;
}

float UIVirtualGamepad::GetUnit() const {
    const UIRectF content = GetContentRect();
    return std::min(content.Size.x / s_DesignSize.x, content.Size.y / s_DesignSize.y) * Scale;
}

Vec2 UIVirtualGamepad::ToLocal(const Vec2& InDesign) const {
    const UIRectF content = GetContentRect();
    const float unit = GetUnit();

    float x = content.Center().x + (InDesign.x - s_DesignSize.x * 0.5f) * unit;
    if (InDesign.x < s_LeftColumnEnd) {
        x = content.Min().x + InDesign.x * unit;
    } else if (InDesign.x > s_RightColumnStart) {
        x = content.Max().x - (s_DesignSize.x - InDesign.x) * unit;
    }

    const float y = InDesign.y < s_TopRowEnd
        ? content.Min().y + InDesign.y * unit
        : content.Max().y - (s_DesignSize.y - InDesign.y) * unit;
    return Vec2(x, y);
}

Vec2 UIVirtualGamepad::ToScreen(const Vec2& InDesign) const {
    return LocalToScreen(ToLocal(InDesign));
}

float UIVirtualGamepad::ToScreenRadius(const Vec2& InDesign, float InRadius) const {
    const Vec2 center = ToLocal(InDesign);
    return glm::distance(LocalToScreen(center), LocalToScreen(center + Vec2(InRadius * GetUnit(), 0.0f)));
}

Array<UIVirtualGamepad::Pointer> UIVirtualGamepad::GatherPointers(const UIFrameContext& InContext) const {
    Array<Pointer> pointers;
    if (MultiTouchDevice* touchscreen = MultiTouchDevice::Instance()) {
        for (const TouchPoint& touch : touchscreen->GetTouches()) {
            if (touch.IsActive()) {
                pointers.Add({ touch.Id, touch.Position, touch.BeganThisFrame });
            }
        }
        return pointers;
    }

    if (InContext.CursorDown && InContext.CursorPosition.x > s_OffscreenCursor) {
        pointers.Add({ 0, InContext.CursorPosition, InContext.CursorPressedThisFrame });
    }
    return pointers;
}

const UIVirtualGamepad::Pointer* UIVirtualGamepad::FindCapturedPointer(const Capture& InCapture, const Array<Pointer>& InPointers) const {
    if (!InCapture.Active) {
        return nullptr;
    }
    for (const Pointer& pointer : InPointers) {
        if (pointer.Id == InCapture.PointerId) {
            return &pointer;
        }
    }
    return nullptr;
}

bool UIVirtualGamepad::IsCaptured(uint64_t InPointerId) const {
    for (const Capture* capture : { &m_DPadCapture, &m_LeftStickCapture, &m_RightStickCapture }) {
        if (capture->Active && capture->PointerId == InPointerId) {
            return true;
        }
    }
    return false;
}

void UIVirtualGamepad::UpdateCaptures(const Array<Pointer>& InPointers) {
    struct Region {
        Capture* Target;
        Vec2 Center;
        float Radius;
    };
    const Region regions[] = {
        { &m_LeftStickCapture, s_LeftStickCenter, s_StickOuterRadius },
        { &m_RightStickCapture, s_RightStickCenter, s_StickOuterRadius },
        { &m_DPadCapture, s_DPadCenter, s_DPadRegion },
    };

    for (const Region& region : regions) {
        if (!FindCapturedPointer(*region.Target, InPointers)) {
            *region.Target = Capture();
        }
    }

    for (const Pointer& pointer : InPointers) {
        if (!pointer.Began || IsCaptured(pointer.Id)) {
            continue;
        }
        for (const Region& region : regions) {
            if (region.Target->Active) {
                continue;
            }
            if (glm::distance(pointer.Position, ToScreen(region.Center)) <= ToScreenRadius(region.Center, region.Radius)) {
                *region.Target = Capture{ true, pointer.Id };
                break;
            }
        }
    }
}

Vec2 UIVirtualGamepad::ReadStick(const Capture& InCapture, const Array<Pointer>& InPointers, const Vec2& InCenter) const {
    const Pointer* pointer = FindCapturedPointer(InCapture, InPointers);
    const float travel = ToScreenRadius(InCenter, s_StickTravel);
    if (!pointer || travel <= 0.0f) {
        return Vec2(0.0f);
    }
    Vec2 value = (pointer->Position - ToScreen(InCenter)) / travel;
    const float length = glm::length(value);
    if (length > 1.0f) {
        value /= length;
    }
    return Vec2(value.x, -value.y);
}

void UIVirtualGamepad::ReadDPad(const Array<Pointer>& InPointers, VirtualGamepadState& OutState) const {
    const Pointer* pointer = FindCapturedPointer(m_DPadCapture, InPointers);
    if (!pointer) {
        return;
    }
    const Vec2 offset = pointer->Position - ToScreen(s_DPadCenter);
    if (glm::length(offset) < ToScreenRadius(s_DPadCenter, s_DPadRegion) * s_DPadDeadZone) {
        return;
    }

    const float sector = std::numbers::pi_v<float> / 8.0f;
    const Vec2 direction = glm::normalize(offset);
    for (const DPadSpec& pad : s_DPad) {
        const float angle = std::acos(std::clamp(glm::dot(direction, pad.Direction), -1.0f, 1.0f));
        if (angle < sector * 3.0f) {
            OutState.SetButton(pad.Code, true);
        }
    }
}

void UIVirtualGamepad::OnUIUpdate(const UIFrameContext& InContext) {
    VirtualGamepadState state;
    m_PressedButtons = 0;

    if (IsVisible()) {
        const Array<Pointer> pointers = GatherPointers(InContext);
        UpdateCaptures(pointers);

        for (int32_t i = 0; i < s_ButtonCount; i++) {
            const ButtonSpec& button = s_Buttons[i];
            const Vec2 center = ToScreen(button.Center);
            const Vec2 extent = Vec2(ToScreenRadius(button.Center, button.Extent.x), ToScreenRadius(button.Center, button.Extent.y)) * s_ButtonHitScale;
            bool pressed = false;
            for (const Pointer& pointer : pointers) {
                const Vec2 offset = glm::abs(pointer.Position - center);
                const bool inside = button.Kind == Shape::Circle
                    ? glm::length(offset) <= extent.x
                    : offset.x <= extent.x && offset.y <= extent.y;
                pressed = pressed || (inside && !IsCaptured(pointer.Id));
            }
            if (!pressed) {
                continue;
            }
            m_PressedButtons |= 1u << i;
            switch (button.Target) {
                case Output::LeftTrigger:  state.LeftTrigger = 1.0f; break;
                case Output::RightTrigger: state.RightTrigger = 1.0f; break;
                case Output::Button:       state.SetButton(button.Code, true); break;
            }
        }

        state.LeftStick = ReadStick(m_LeftStickCapture, pointers, s_LeftStickCenter);
        state.RightStick = ReadStick(m_RightStickCapture, pointers, s_RightStickCenter);
        ReadDPad(pointers, state);
    } else {
        m_DPadCapture = Capture();
        m_LeftStickCapture = Capture();
        m_RightStickCapture = Capture();
    }

    m_State = state;
    if (VirtualGamepadDevice* device = m_Device.Get()) {
        device->Submit(state);
    }
}

Vec4 UIVirtualGamepad::GetLineColor() const {
    return Vec4(Vec3(Tint), Tint.a * Opacity);
}

Vec4 UIVirtualGamepad::GetFillColor() const {
    return Vec4(Vec3(Tint), Tint.a * Opacity * 0.35f);
}

void UIVirtualGamepad::Paint(UIDrawList& OutDrawList) {
    if (!IsVisible()) {
        return;
    }
    const float unit = GetUnit();
    for (int32_t i = 0; i < s_ButtonCount; i++) {
        PaintButton(OutDrawList, i, unit);
    }
    PaintDPad(OutDrawList, unit);
    PaintStick(OutDrawList, s_LeftStickCenter, m_State.LeftStick, unit);
    PaintStick(OutDrawList, s_RightStickCenter, m_State.RightStick, unit);
}

void UIVirtualGamepad::PaintButton(UIDrawList& OutDrawList, int32_t InIndex, float InUnit) const {
    const ButtonSpec& button = s_Buttons[InIndex];
    const Vec2 center = ToLocal(button.Center);
    const Vec2 extent = button.Extent * InUnit;
    const float stroke = std::max(1.5f, 3.0f * InUnit);
    const bool pressed = (m_PressedButtons & (1u << InIndex)) != 0;

    if (button.Kind == Shape::Pill) {
        const UIRectF rect(center - extent, extent * 2.0f);
        if (pressed) {
            OutDrawList.AddRoundedRect(rect, GetFillColor(), extent.y, m_WorldMatrix);
        }
        OutDrawList.AddRoundedRectOutline(rect, GetLineColor(), extent.y, stroke, m_WorldMatrix);
    } else {
        if (pressed) {
            OutDrawList.AddCircle(center, extent.x, GetFillColor(), s_Segments, m_WorldMatrix);
        }
        OutDrawList.AddRing(center, extent.x, stroke, GetLineColor(), s_Segments, m_WorldMatrix);
    }

    const float arrow = 16.0f * InUnit;
    switch (button.Glyph) {
        case Icon::Label: {
            const bool face = button.Label[1] == '\0';
            PaintLabel(OutDrawList, button.Label, center, (face ? s_FaceLabelHeight : s_ShoulderLabelHeight) * InUnit);
            break;
        }
        case Icon::ArrowLeft: {
            const Vec2 points[3] = { center + Vec2(-arrow, 0.0f), center + Vec2(arrow, arrow), center + Vec2(arrow, -arrow) };
            OutDrawList.AddConvexPolyFilled(points, 3, GetLineColor(), m_WorldMatrix);
            break;
        }
        case Icon::ArrowRight: {
            const Vec2 points[3] = { center + Vec2(-arrow, -arrow), center + Vec2(-arrow, arrow), center + Vec2(arrow, 0.0f) };
            OutDrawList.AddConvexPolyFilled(points, 3, GetLineColor(), m_WorldMatrix);
            break;
        }
        case Icon::Controller:
            PaintController(OutDrawList, center, InUnit);
            break;
    }
}

void UIVirtualGamepad::PaintDPad(UIDrawList& OutDrawList, float InUnit) const {
    const float radius = s_ButtonRadius * InUnit;
    const float stroke = std::max(1.5f, 3.0f * InUnit);
    for (const DPadSpec& pad : s_DPad) {
        const Vec2 center = ToLocal(s_DPadCenter) + pad.Direction * s_DPadSpacing * InUnit;
        if (m_State.IsButtonPressed(pad.Code)) {
            OutDrawList.AddCircle(center, radius, GetFillColor(), s_Segments, m_WorldMatrix);
        }
        OutDrawList.AddRing(center, radius, stroke, GetLineColor(), s_Segments, m_WorldMatrix);
        PaintChevron(OutDrawList, center, pad.Direction, 14.0f * InUnit, stroke * 1.4f);
    }
}

void UIVirtualGamepad::PaintStick(UIDrawList& OutDrawList, const Vec2& InCenter, const Vec2& InValue, float InUnit) const {
    const Vec2 center = ToLocal(InCenter);
    const float stroke = std::max(1.5f, 3.0f * InUnit);
    const Vec4 line = GetLineColor();
    OutDrawList.AddRing(center, s_StickOuterRadius * InUnit, stroke, line, s_Segments, m_WorldMatrix);
    OutDrawList.AddRing(center, s_StickInnerRadius * InUnit, stroke, Vec4(Vec3(line), line.a * 0.35f), s_Segments, m_WorldMatrix);

    const Vec2 knob = center + Vec2(InValue.x, -InValue.y) * s_StickTravel * InUnit;
    OutDrawList.AddCircle(knob, s_StickKnobRadius * InUnit, Vec4(Vec3(Tint), Tint.a * Opacity), s_Segments, m_WorldMatrix);

    const float spacing = 11.0f * InUnit;
    const Vec4 dot = Vec4(0.0f, 0.0f, 0.0f, 0.3f * Opacity);
    for (int32_t y = -3; y <= 3; y++) {
        for (int32_t x = -3; x <= 3; x++) {
            if (x * x + y * y > 10) {
                continue;
            }
            OutDrawList.AddCircle(knob + Vec2((float)x, (float)y) * spacing, 2.2f * InUnit, dot, 8, m_WorldMatrix);
        }
    }
}

void UIVirtualGamepad::PaintLabel(UIDrawList& OutDrawList, const String& InText, const Vec2& InCenter, float InHeight) const {
    Font* font = GetDefaultFont();
    if (!font || !font->IsLoaded()) {
        return;
    }
    const float width = font->GetTextWidth(InText, InHeight);
    const float ascent = font->GetAscentBasePx() * font->GetScaleForPixelHeight(InHeight);
    const Vec2 topLeft = Vec2(InCenter.x - width * 0.5f, InCenter.y - ascent * 0.625f);
    OutDrawList.AddText(font, InText, topLeft, InHeight, GetLineColor(), m_WorldMatrix);
}

void UIVirtualGamepad::PaintChevron(UIDrawList& OutDrawList, const Vec2& InCenter, const Vec2& InDirection, float InSize, float InThickness) const {
    const Vec2 side = Vec2(-InDirection.y, InDirection.x);
    const Vec2 tip = InCenter + InDirection * InSize * 0.5f;
    const Vec2 base = InCenter - InDirection * InSize * 0.5f;
    OutDrawList.AddLine(base + side * InSize, tip, InThickness, GetLineColor(), m_WorldMatrix);
    OutDrawList.AddLine(tip, base - side * InSize, InThickness, GetLineColor(), m_WorldMatrix);
}

void UIVirtualGamepad::PaintController(UIDrawList& OutDrawList, const Vec2& InCenter, float InUnit) const {
    const Vec4 line = GetLineColor();
    const Vec4 cutout = Vec4(0.0f, 0.0f, 0.0f, 0.55f * Opacity);
    const Vec2 body = Vec2(26.0f, 13.0f) * InUnit;
    const Vec2 center = InCenter - Vec2(0.0f, 5.0f * InUnit);

    OutDrawList.AddRoundedRect(UIRectF(center - body, body * 2.0f), line, body.y, m_WorldMatrix);
    OutDrawList.AddCircle(center + Vec2(-17.0f, 10.0f) * InUnit, 9.0f * InUnit, line, 16, m_WorldMatrix);
    OutDrawList.AddCircle(center + Vec2(17.0f, 10.0f) * InUnit, 9.0f * InUnit, line, 16, m_WorldMatrix);

    const Vec2 cross = center + Vec2(-12.0f, -1.0f) * InUnit;
    const float arm = 5.0f * InUnit;
    const float thickness = 2.6f * InUnit;
    OutDrawList.AddLine(cross - Vec2(arm, 0.0f), cross + Vec2(arm, 0.0f), thickness, cutout, m_WorldMatrix);
    OutDrawList.AddLine(cross - Vec2(0.0f, arm), cross + Vec2(0.0f, arm), thickness, cutout, m_WorldMatrix);
    OutDrawList.AddCircle(center + Vec2(10.0f, -4.0f) * InUnit, 2.6f * InUnit, cutout, 8, m_WorldMatrix);
    OutDrawList.AddCircle(center + Vec2(15.0f, 2.0f) * InUnit, 2.6f * InUnit, cutout, 8, m_WorldMatrix);

    const float chevron = 6.0f * InUnit;
    const Vec2 below = InCenter + Vec2(0.0f, 30.0f * InUnit);
    const Vec2 points[3] = { below + Vec2(-chevron, -chevron * 0.6f), below + Vec2(0.0f, chevron * 0.6f), below + Vec2(chevron, -chevron * 0.6f) };
    OutDrawList.AddConvexPolyFilled(points, 3, line, m_WorldMatrix);
}

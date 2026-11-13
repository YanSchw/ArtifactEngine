#pragma once
#include "GameFramework/UINode.h"
#include "Object/Pointer.h"
#include "VirtualGamepadDevice.h"
#include "UIVirtualGamepad.gen.h"

class UIVirtualGamepad : public UINode {
public:
    ARTIFACT_CLASS();

    UIVirtualGamepad();

    PROPERTY(Range = (0.25, 3))
    float Scale = 1.0f;

    PROPERTY(Range = (0, 1))
    float Opacity = 0.85f;

    PROPERTY()
    Color Tint = Color(0.86f, 0.86f, 0.86f, 1.0f);

    PROPERTY()
    bool HideWithoutTouchscreen = false;

    const VirtualGamepadState& GetState() const { return m_State; }

    virtual void BeginPlay() override;
    virtual void EndPlay() override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;
    virtual void Paint(UIDrawList& OutDrawList) override;

private:
    struct Pointer {
        uint64_t Id = 0;
        Vec2 Position = Vec2(0.0f);
        bool Began = false;
    };

    struct Capture {
        bool Active = false;
        uint64_t PointerId = 0;
    };

    bool IsVisible() const;
    float GetUnit() const;
    Vec2 ToLocal(const Vec2& InDesign) const;
    Vec2 ToScreen(const Vec2& InDesign) const;
    float ToScreenRadius(const Vec2& InDesign, float InRadius) const;
    Array<Pointer> GatherPointers(const UIFrameContext& InContext) const;
    void UpdateCaptures(const Array<Pointer>& InPointers);
    const Pointer* FindCapturedPointer(const Capture& InCapture, const Array<Pointer>& InPointers) const;
    bool IsCaptured(uint64_t InPointerId) const;
    Vec2 ReadStick(const Capture& InCapture, const Array<Pointer>& InPointers, const Vec2& InCenter) const;
    void ReadDPad(const Array<Pointer>& InPointers, VirtualGamepadState& OutState) const;

    void PaintButton(UIDrawList& OutDrawList, int32_t InIndex, float InUnit) const;
    void PaintDPad(UIDrawList& OutDrawList, float InUnit) const;
    void PaintStick(UIDrawList& OutDrawList, const Vec2& InCenter, const Vec2& InValue, float InUnit) const;
    void PaintLabel(UIDrawList& OutDrawList, const String& InText, const Vec2& InCenter, float InHeight) const;
    void PaintChevron(UIDrawList& OutDrawList, const Vec2& InCenter, const Vec2& InDirection, float InSize, float InThickness) const;
    void PaintController(UIDrawList& OutDrawList, const Vec2& InCenter, float InUnit) const;
    Vec4 GetLineColor() const;
    Vec4 GetFillColor() const;

    WeakObjectPtr<VirtualGamepadDevice> m_Device;
    VirtualGamepadState m_State;
    uint32_t m_PressedButtons = 0;
    Capture m_DPadCapture;
    Capture m_LeftStickCapture;
    Capture m_RightStickCapture;
};

#pragma once
#include "ThemedWindow.h"
#include <functional>
#include "ColorPickerWindow.gen.h"

class UIColorSlider;
class UIColorWheel;
class UITextArea;

class ColorPickerWindow : public ThemedWindow {
public:
    ARTIFACT_CLASS();
protected:
    ColorPickerWindow(const WindowParams& InParams);
public:
    ~ColorPickerWindow();

    using ApplyFn = std::function<void(const Color&)>;

    static ColorPickerWindow* Open(const Color& InColor, bool InUseAlpha, const String& InTitle,
                                   const Vec2& InScreenPos, ApplyFn InApply);

    virtual bool OnCloseRequested() override;

private:
    void BuildContent();
    void BuildWheelRow(UINode& InParent);
    void BuildFields(UINode& InParent);
    void BuildButtons(UINode& InParent);

    Color GetColor() const;
    void SetColor(const Color& InColor);
    void Apply();
    void Accept();
    void Revert();

    Vec3 m_Hsv = Vec3(0.0f, 0.0f, 1.0f);
    float m_Alpha = 1.0f;
    Color m_Original = Color(1.0f);
    bool m_UseAlpha = true;
    bool m_Closing = false;
    ApplyFn m_Apply;

    UIColorWheel* m_Wheel = nullptr;
    UIColorSlider* m_ValueSlider = nullptr;
    UIColorSlider* m_AlphaSlider = nullptr;
    UITextArea* m_HexField = nullptr;
};

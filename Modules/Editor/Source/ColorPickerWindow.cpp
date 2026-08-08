#include "ColorPickerWindow.h"

#include "UI/EditorStyle.h"
#include "UI/UIColorSlider.h"
#include "UI/UIColorSwatch.h"
#include "UI/UIColorWheel.h"
#include "UI/UIDragNumber.h"
#include "Common/Color.h"
#include "GameFramework/UIBuilder.h"
#include "GameFramework/UIButton.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UIQuad.h"
#include "GameFramework/UITextArea.h"

static constexpr float s_Margin = 12.0f;
static constexpr float s_WheelSize = 176.0f;
static constexpr float s_SliderWidth = 22.0f;
static constexpr float s_FieldHeight = 22.0f;
static constexpr float s_FieldSpacing = 4.0f;
static constexpr float s_LabelWidth = 18.0f;

static ColorPickerWindow* s_OpenPicker = nullptr;

ColorPickerWindow::ColorPickerWindow(const WindowParams& InParams)
    : ThemedWindow(InParams) {
}

ColorPickerWindow::~ColorPickerWindow() {
    if (s_OpenPicker == this) {
        s_OpenPicker = nullptr;
    }
}

ColorPickerWindow* ColorPickerWindow::Open(const Color& InColor, bool InUseAlpha, const String& InTitle,
                                           const Vec2& InScreenPos, ApplyFn InApply) {
    if (s_OpenPicker) {
        ThemedWindow::DestroyWindow(s_OpenPicker);
    }

    WindowParams params;
    params.Title = InTitle;
    params.Width = 448;
    params.Height = 344 + (uint32_t)EditorStyle::TitleBarHeight;
    params.EditorStyle = true;

    SharedObjectPtr<ColorPickerWindow> window(new ColorPickerWindow(params));
    window->m_Original = InColor;
    window->m_UseAlpha = InUseAlpha;
    window->m_Apply = std::move(InApply);
    window->SetColor(InColor);
    window->SetPosition(InScreenPos + Vec2(8.0f, 8.0f));
    window->BuildContent();

    ThemedWindow::RegisterWindow(window);
    s_OpenPicker = window.Get();
    return s_OpenPicker;
}

Color ColorPickerWindow::GetColor() const {
    return Color(ColorUtils::HsvToRgb(m_Hsv), m_UseAlpha ? m_Alpha : 1.0f);
}

void ColorPickerWindow::SetColor(const Color& InColor) {
    const Vec3 hsv = ColorUtils::RgbToHsv(glm::clamp(Vec3(InColor), Vec3(0.0f), Vec3(1.0f)));
    m_Hsv = Vec3(hsv.y > 0.0f ? hsv.x : m_Hsv.x, hsv.y, hsv.z);
    m_Alpha = glm::clamp(InColor.a, 0.0f, 1.0f);
}

void ColorPickerWindow::Apply() {
    if (m_Apply) {
        m_Apply(GetColor());
    }
}

void ColorPickerWindow::Accept() {
    m_Closing = true;
    Close();
}

void ColorPickerWindow::Revert() {
    m_Closing = true;
    if (m_Apply) {
        m_Apply(m_Original);
    }
    Close();
}

bool ColorPickerWindow::OnCloseRequested() {
    if (!m_Closing && m_Apply) {
        m_Apply(m_Original);
    }
    return true;
}

void ColorPickerWindow::BuildContent() {
    UIQuad* background = m_ContentRoot->Add<UIQuad>();
    background->Fill();
    background->Color = EditorStyle::Panel;

    BuildWheelRow(*background);
    BuildFields(*background);
    BuildButtons(*background);
}

void ColorPickerWindow::BuildWheelRow(UINode& InParent) {
    const auto topLeft = [&InParent](UINode& InNode, float InX, float InY) {
        (void)InParent;
        InNode.Anchor = InNode.Pivot = Vec2(0.0f);
        InNode.Position = Vec2(InX, InY);
    };

    m_Wheel = InParent.Add<UIColorWheel>();
    topLeft(*m_Wheel, s_Margin, s_Margin);
    m_Wheel->Size = Vec2(s_WheelSize);
    m_Wheel->Changed = [this] {
        m_Hsv.x = m_Wheel->Hue;
        m_Hsv.y = m_Wheel->Saturation;
        Apply();
    };
    m_Wheel->Bind = [this] {
        m_Wheel->Hue = m_Hsv.x;
        m_Wheel->Saturation = m_Hsv.y;
        m_Wheel->Value = m_Hsv.z;
    };

    m_ValueSlider = InParent.Add<UIColorSlider>();
    topLeft(*m_ValueSlider, s_Margin + s_WheelSize + 14.0f, s_Margin);
    m_ValueSlider->Size = Vec2(s_SliderWidth, s_WheelSize);
    m_ValueSlider->Changed = [this] {
        m_Hsv.z = m_ValueSlider->Value;
        Apply();
    };
    m_ValueSlider->Bind = [this] {
        m_ValueSlider->Value = m_Hsv.z;
        m_ValueSlider->TopColor = Color(ColorUtils::HsvToRgb(Vec3(m_Hsv.x, m_Hsv.y, 1.0f)), 1.0f);
        m_ValueSlider->BottomColor = Color(0.0f, 0.0f, 0.0f, 1.0f);
    };

    if (m_UseAlpha) {
        m_AlphaSlider = InParent.Add<UIColorSlider>();
        topLeft(*m_AlphaSlider, s_Margin + s_WheelSize + 14.0f + s_SliderWidth + 8.0f, s_Margin);
        m_AlphaSlider->Size = Vec2(s_SliderWidth, s_WheelSize);
        m_AlphaSlider->Translucent = true;
        m_AlphaSlider->Changed = [this] {
            m_Alpha = m_AlphaSlider->Value;
            Apply();
        };
        m_AlphaSlider->Bind = [this] {
            m_AlphaSlider->Value = m_Alpha;
            m_AlphaSlider->TopColor = Color(Vec3(GetColor()), 1.0f);
            m_AlphaSlider->BottomColor = Color(Vec3(GetColor()), 0.0f);
        };
    }

    const float previewX = s_Margin + s_WheelSize + 14.0f + (s_SliderWidth + 8.0f) * 2.0f + 8.0f;
    const auto addPreview = [&](const String& InCaption, float InY, std::function<Color()> InGet) {
        UILabel* caption = InParent.Add<UILabel>();
        topLeft(*caption, previewX, InY);
        caption->Size = Vec2(100.0f, 14.0f);
        caption->Text = InCaption;
        caption->FontSize = EditorStyle::FontSize;
        caption->Color = EditorStyle::TextDim;

        UIColorSwatch* swatch = InParent.Add<UIColorSwatch>();
        topLeft(*swatch, previewX, InY + 16.0f);
        swatch->Size = Vec2(100.0f, 34.0f);
        swatch->Interactable = false;
        swatch->UseAlpha = m_UseAlpha;
        swatch->Get = std::move(InGet);
    };

    addPreview("Old", s_Margin, [this] { return m_Original; });
    addPreview("New", s_Margin + 62.0f, [this] { return GetColor(); });
}

void ColorPickerWindow::BuildFields(UINode& InParent) {
    const float rowTop = s_Margin + s_WheelSize + 16.0f;
    const float columnWidth = 190.0f;

    const auto addField = [&](const String& InLabel, float InColumn, int32_t InRow, double InMax,
                              std::function<double()> InGet, std::function<void(double)> InSet) {
        const float x = s_Margin + InColumn * (columnWidth + 24.0f);
        const float y = rowTop + (float)InRow * (s_FieldHeight + s_FieldSpacing);

        UILabel* label = InParent.Add<UILabel>();
        label->Anchor = label->Pivot = Vec2(0.0f);
        label->Position = Vec2(x, y);
        label->Size = Vec2(s_LabelWidth, s_FieldHeight);
        label->Text = InLabel;
        label->FontSize = EditorStyle::FontSize;
        label->Color = EditorStyle::TextDim;
        label->VAlign = UIVAlign::Middle;

        UIDragNumber* drag = InParent.Add<UIDragNumber>();
        drag->Anchor = drag->Pivot = Vec2(0.0f);
        drag->Position = Vec2(x + s_LabelWidth, y);
        drag->Size = Vec2(columnWidth - s_LabelWidth, s_FieldHeight);
        drag->Decimals = InMax > 1.0 ? 1 : 3;
        drag->Sensitivity = InMax > 1.0 ? 0.5 : 0.005;
        drag->MinValue = 0.0;
        drag->MaxValue = InMax;
        drag->Get = std::move(InGet);
        drag->Set = std::move(InSet);
    };

    static const char* s_Channels[] = { "R", "G", "B", "A" };
    for (int32_t i = 0; i < (m_UseAlpha ? 4 : 3); i++) {
        addField(s_Channels[i], 0.0f, i, 1.0,
                 [this, i] { return (double)GetColor()[i]; },
                 [this, i](double InValue) {
                     Color color = GetColor();
                     color[i] = (float)InValue;
                     SetColor(color);
                     Apply();
                 });
    }

    static const char* s_Axes[] = { "H", "S", "V" };
    for (int32_t i = 0; i < 3; i++) {
        addField(s_Axes[i], 1.0f, i, i == 0 ? 360.0 : 1.0,
                 [this, i] { return (double)m_Hsv[i]; },
                 [this, i](double InValue) {
                     m_Hsv[i] = (float)InValue;
                     Apply();
                 });
    }

    UILabel* hexLabel = InParent.Add<UILabel>();
    hexLabel->Anchor = hexLabel->Pivot = Vec2(0.0f);
    hexLabel->Position = Vec2(s_Margin + columnWidth + 24.0f, rowTop + 3.0f * (s_FieldHeight + s_FieldSpacing));
    hexLabel->Size = Vec2(30.0f, s_FieldHeight);
    hexLabel->Text = "Hex";
    hexLabel->FontSize = EditorStyle::FontSize;
    hexLabel->Color = EditorStyle::TextDim;
    hexLabel->VAlign = UIVAlign::Middle;

    m_HexField = InParent.Add<UITextArea>();
    m_HexField->Anchor = m_HexField->Pivot = Vec2(0.0f);
    m_HexField->Position = Vec2(s_Margin + columnWidth + 24.0f + 30.0f, rowTop + 3.0f * (s_FieldHeight + s_FieldSpacing));
    m_HexField->Size = Vec2(columnWidth - 30.0f, s_FieldHeight);
    m_HexField->SingleLine = true;
    m_HexField->FontSize = EditorStyle::FontSize;
    m_HexField->TextColor = EditorStyle::Text;
    m_HexField->CaretColor = EditorStyle::TextBright;
    m_HexField->BackgroundColor = EditorStyle::PanelDark;
    m_HexField->FocusedBorderColor = EditorStyle::Accent;
    m_HexField->Padding = UIPadding(6.0f, 0.0f);
    m_HexField->Bind = [this] {
        if (!m_HexField->IsFocused()) {
            m_HexField->Text = ColorUtils::ToHex(GetColor(), m_UseAlpha);
        }
    };

    const auto commitHex = [this](const String& InText) {
        Color parsed;
        if (ColorUtils::FromHex(InText, parsed)) {
            SetColor(m_UseAlpha ? parsed : Color(Vec3(parsed), 1.0f));
            Apply();
        }
    };
    m_HexField->Submitted = commitHex;
    m_HexField->FocusLost = [this, commitHex] { commitHex(m_HexField->Text); };
}

void ColorPickerWindow::BuildButtons(UINode& InParent) {
    const auto addButton = [&](const String& InCaption, float InRight, std::function<void()> InClicked) {
        UIButton& button = UI::Button(InParent, InCaption, std::move(InClicked));
        button.Anchor = button.Pivot = Vec2(1.0f);
        button.Position = Vec2(-InRight, -s_Margin);
        button.Size = Vec2(74.0f, 26.0f);
        EditorStyle::ApplyButtonStyle(button);
    };

    addButton("Cancel", s_Margin, [this] { Revert(); });
    addButton("OK", s_Margin + 82.0f, [this] { Accept(); });
}

#include "UIModalDialog.h"
#include "EditorStyle.h"
#include "EditorIcons.h"
#include "UIRoundedQuad.h"
#include "GameFramework/UICanvas.h"
#include "GameFramework/UIVStack.h"
#include "GameFramework/UIHStack.h"
#include "GameFramework/UIQuad.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UIButton.h"
#include "GameFramework/UISvg.h"
#include "InputSystem/KeyboardDevice.h"
#include "Rendering/UIDrawList.h"

static const Vec4 s_ScrimColor = Vec4(0.0f, 0.0f, 0.0f, 0.55f);
static constexpr float s_TitleBarHeight = 32.0f;
static constexpr float s_FooterHeight = 46.0f;
static constexpr float s_CaptionWidth = 96.0f;

UIModalDialog::UIModalDialog() {
    Fill();
    Interactable = true;
}

UIModalDialog* UIModalDialog::Open(UINode& InOwner, const String& InTitle, const Vec2& InSize) {
    UICanvas* canvas = InOwner.GetCanvas();
    if (!canvas) {
        return nullptr;
    }
    UIModalDialog* dialog = canvas->Add<UIModalDialog>();
    dialog->Build(InTitle, InSize);
    return dialog;
}

void UIModalDialog::Build(const String& InTitle, const Vec2& InSize) {
    UIRoundedQuad* border = Add<UIRoundedQuad>();
    border->Center(InSize);
    border->Color = EditorStyle::Border;
    border->CornerRadius = Vec4(7.0f);
    m_Panel = border;

    UIRoundedQuad* panel = border->Add<UIRoundedQuad>();
    panel->Center({ 1.0_rel - 2.0_px, 1.0_rel - 2.0_px });
    panel->Color = EditorStyle::Panel;
    panel->CornerRadius = Vec4(6.0f);

    UIVStack* column = panel->Add<UIVStack>();
    column->Fill();

    UIRoundedQuad* titleBar = column->Add<UIRoundedQuad>();
    titleBar->Size = { 1.0_rel, UIValue(s_TitleBarHeight) };
    titleBar->Color = EditorStyle::ToolBar;
    titleBar->CornerRadius = Vec4(6.0f, 6.0f, 0.0f, 0.0f);

    UILabel* title = titleBar->Add<UILabel>();
    title->Fill();
    title->Padding = UIPadding(12.0f, 0.0f);
    title->FontSize = EditorStyle::FontSize;
    title->VAlign = UIVAlign::Middle;
    title->Color = EditorStyle::TextBright;
    title->Text = InTitle;

    UIButton* close = titleBar->Add<UIButton>();
    close->Anchor = close->Pivot = Vec2(1.0f, 0.5f);
    close->Position = Vec2(-2.0f, 0.0f);
    close->Size = Vec2(28.0f, 24.0f);
    close->NormalColor = Vec4(0.0f);
    close->HoverColor = EditorStyle::CaptionCloseHover;
    close->PressedColor = EditorStyle::ButtonPressed;
    close->Clicked = [this] { Close(); };
    UISvg* closeIcon = close->Add<UISvg>();
    closeIcon->Center(Vec2(9.0f, 9.0f));
    closeIcon->Image = EditorIcons::Close();
    closeIcon->Tint = EditorStyle::Text;

    m_Body = column->Add<UIVStack>();
    m_Body->Size = { 1.0_rel, 1.0_rel };
    m_Body->Padding = UIPadding(14.0f, 12.0f);
    m_Body->Gap = 8.0f;

    UIQuad* footer = column->Add<UIQuad>();
    footer->Size = { 1.0_rel, UIValue(s_FooterHeight) };
    footer->Color = EditorStyle::ToolBar;

    m_Buttons = footer->Add<UIHStack>();
    m_Buttons->Fill();
    m_Buttons->Padding = UIPadding(12.0f, 10.0f);
    m_Buttons->Gap = 8.0f;

    // Weighted spacer, so every button added after it is pushed against the right edge.
    UINode* spacer = m_Buttons->Add<UINode>();
    spacer->Size = { 1.0_rel, 1.0_rel };
}

UINode* UIModalDialog::AddField(const String& InCaption, float InHeight) {
    UIHStack* row = m_Body->Add<UIHStack>();
    row->Size = { 1.0_rel, UIValue(InHeight) };
    row->Gap = 8.0f;

    UILabel* caption = row->Add<UILabel>();
    caption->Size = { UIValue(s_CaptionWidth), 1.0_rel };
    caption->FontSize = EditorStyle::FontSize;
    caption->VAlign = UIVAlign::Middle;
    caption->Color = EditorStyle::TextDim;
    caption->Text = InCaption;

    UINode* control = row->Add<UINode>();
    control->Size = { 1.0_rel, 1.0_rel };
    return control;
}

void UIModalDialog::AddButton(const String& InLabel, bool InPrimary, std::function<void()> InClicked) {
    UIButton* button = m_Buttons->Add<UIButton>();
    button->Size = { 84.0_px, 1.0_rel };
    button->SetCaption(InLabel);
    EditorStyle::ApplyButtonStyle(*button);
    if (InPrimary) {
        button->NormalColor = EditorStyle::Accent;
        button->HoverColor = EditorStyle::AccentBright;
    }
    button->Clicked = std::move(InClicked);
}

void UIModalDialog::Close() {
    if (m_Closing) {
        return;
    }
    m_Closing = true;
    if (UICanvas* canvas = GetCanvas()) {
        canvas->DestroyDeferred(this);
    }
}

void UIModalDialog::Paint(UIDrawList& OutDrawList) {
    OutDrawList.AddRect(m_Geometry, s_ScrimColor, m_WorldMatrix);
}

void UIModalDialog::OnUIUpdate(const UIFrameContext& InContext) {
    (void)InContext;
    KeyboardDevice* keyboard = KeyboardDevice::Instance();
    if (keyboard && keyboard->IsDown(KeyCode::Escape)) {
        Close();
    }
}

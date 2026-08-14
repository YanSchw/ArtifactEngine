#include "DialogWindow.h"

#include "UI/EditorStyle.h"
#include "GameFramework/UIButton.h"
#include "GameFramework/UIHStack.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UIQuad.h"
#include "GameFramework/UITextArea.h"
#include "GameFramework/UIVStack.h"
#include "InputSystem/KeyboardDevice.h"
#include "InputSystem/KeyCodes.h"
#include <cctype>

static constexpr float s_Margin = 14.0f;
static constexpr float s_Gap = 8.0f;
static constexpr float s_CaptionHeight = 16.0f;
static constexpr float s_CaptionWidth = 92.0f;
static constexpr float s_ButtonWidth = 84.0f;

DialogWindow::DialogWindow(const WindowParams& InParams)
    : ThemedWindow(InParams) {
}

WindowParams DialogWindow::MakeParams(const String& InTitle, uint32_t InWidth, uint32_t InContentHeight) {
    WindowParams params;
    params.Title = InTitle;
    params.Width = InWidth;
    params.Height = InContentHeight + (uint32_t)EditorStyle::TitleBarHeight;
    params.EditorStyle = true;
    return params;
}

Window* DialogWindow::CurrentOpener() {
    Window* opener = Window::GetFocusedWindow();
    return opener ? opener : Window::GetInstance();
}

void DialogWindow::Present(const SharedObjectPtr<DialogWindow>& InWindow, Window* InOpener) {
    if (InOpener) {
        const Vec2 openerSize((float)InOpener->GetWidth(), (float)InOpener->GetHeight());
        const Vec2 size((float)InWindow->GetWidth(), (float)InWindow->GetHeight());
        InWindow->SetPosition(InOpener->GetPosition() + (openerSize - size) * 0.5f);
    }
    ThemedWindow::RegisterWindow(InWindow);
}

void DialogWindow::BuildFrame() {
    UIQuad* background = m_ContentRoot->Add<UIQuad>();
    background->Fill();
    background->Color = EditorStyle::Panel;

    UIVStack* column = background->Add<UIVStack>();
    column->Fill();
    column->Padding = UIPadding(s_Margin, s_Margin);
    column->Gap = s_Gap;

    m_Body = column->Add<UIVStack>();
    m_Body->Size = { 1.0_rel, 1.0_rel };
    m_Body->Gap = s_Gap;

    m_Footer = column->Add<UIHStack>();
    m_Footer->Size = { 1.0_rel, UIValue(FieldHeight) };
    m_Footer->Gap = s_Gap;

    // Weighted spacer, so every button added after it is pushed against the right edge.
    UINode* spacer = m_Footer->Add<UINode>();
    spacer->Size = { 1.0_rel, 1.0_rel };
}

UILabel* DialogWindow::AddCaption(const String& InText) {
    UILabel* caption = m_Body->Add<UILabel>();
    caption->Size = { 1.0_rel, UIValue(s_CaptionHeight) };
    caption->Text = InText;
    caption->FontSize = EditorStyle::FontSize;
    caption->Color = EditorStyle::TextDim;
    return caption;
}

static void StyleField(UITextArea& InField) {
    InField.SingleLine = true;
    InField.FontSize = EditorStyle::FontSize;
    InField.TextColor = EditorStyle::TextBright;
    InField.CaretColor = EditorStyle::TextBright;
    InField.BackgroundColor = EditorStyle::PanelDark;
    InField.FocusedBorderColor = EditorStyle::Accent;
    InField.Padding = UIPadding(6.0f, 0.0f);
}

UITextArea* DialogWindow::AddTextField(const String& InPlaceholder) {
    UITextArea* field = m_Body->Add<UITextArea>();
    field->Size = { 1.0_rel, UIValue(FieldHeight) };
    field->Placeholder = InPlaceholder;
    StyleField(*field);
    return field;
}

UITextArea* DialogWindow::AddNamedTextField(const String& InCaption) {
    UIHStack* row = m_Body->Add<UIHStack>();
    row->Size = { 1.0_rel, UIValue(FieldHeight) };
    row->Gap = s_Gap;

    UILabel* caption = row->Add<UILabel>();
    caption->Size = { UIValue(s_CaptionWidth), 1.0_rel };
    caption->Text = InCaption;
    caption->FontSize = EditorStyle::FontSize;
    caption->VAlign = UIVAlign::Middle;
    caption->Color = EditorStyle::TextDim;

    UITextArea* field = row->Add<UITextArea>();
    field->Size = { 1.0_rel, 1.0_rel };
    StyleField(*field);
    return field;
}

UINode* DialogWindow::AddFillPanel() {
    UIQuad* panel = m_Body->Add<UIQuad>();
    panel->Size = { 1.0_rel, 1.0_rel };
    panel->Color = EditorStyle::PanelDark;
    return panel;
}

UIButton* DialogWindow::AddFooterButton(const String& InCaption, bool InPrimary, std::function<void()> InClicked) {
    UIButton* button = m_Footer->Add<UIButton>();
    button->Size = { UIValue(s_ButtonWidth), 1.0_rel };
    button->SetCaption(InCaption);
    EditorStyle::ApplyButtonStyle(*button);
    if (InPrimary) {
        button->NormalColor = EditorStyle::Accent;
        button->HoverColor = EditorStyle::AccentBright;
    }
    button->Clicked = std::move(InClicked);
    return button;
}

String DialogWindow::SanitizeName(const String& InName) {
    String name;
    for (char c : InName) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            continue;
        }
        name += c;
    }
    while (!name.empty() && std::isspace((unsigned char)name.front())) { name.erase(name.begin()); }
    while (!name.empty() && std::isspace((unsigned char)name.back())) { name.pop_back(); }
    return name;
}

void DialogWindow::PreUIRender(double InDeltaTime) {
    (void)InDeltaTime;
    KeyboardDevice* keyboard = KeyboardDevice::Instance();
    if (IsFocused() && keyboard && keyboard->IsDown(KeyCode::Escape)) {
        Close();
    }
}

#include "InputActionListTab.h"
#include "InputMappingEditorTab.h"
#include "UI/EditorStyle.h"
#include "UI/EditorIcons.h"
#include "UI/UIContextMenu.h"
#include "GameFramework/UIVStack.h"
#include "GameFramework/UIHStack.h"
#include "GameFramework/UIQuad.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UISvg.h"
#include "GameFramework/UIButton.h"
#include "GameFramework/UIScrollArea.h"
#include "InputSystem/InputActionMapping.h"
#include "InputSystem/InputAction.h"
#include "InputSystem/InputBinding.h"
#include "InputSystem/KeyboardDevice.h"
#include "InputSystem/KeyCodes.h"

static const Vec4 s_SelectedColor = HexColor(0x0F5A9E);
static const Vec4 s_AddGreen = HexColor(0x3C8C3C);
static constexpr float s_HeaderHeight = 30.0f;
static constexpr float s_RowHeight = 22.0f;
static constexpr float s_ActionTextLeft = 28.0f;
static constexpr float s_BindingTextLeft = 40.0f;
static constexpr float s_TypeColumnWidth = 96.0f;

static String GetRowName(Object* InTarget) {
    if (InputAction* action = Cast<InputAction>(InTarget)) {
        return action->Name.empty() ? String("Unnamed Action") : action->Name;
    }
    if (InputBinding* binding = Cast<InputBinding>(InTarget)) {
        return InputMappingEditorTab::DescribeBinding(*binding);
    }
    return String();
}

static String GetRowType(Object* InTarget) {
    if (InputAction* action = Cast<InputAction>(InTarget)) {
        return EInputValueType::ConvertEnumToString(action->Type);
    }
    if (InputBinding* binding = Cast<InputBinding>(InTarget)) {
        return InputMappingEditorTab::GetBindingLabel(binding->GetClass());
    }
    return String();
}

static UIButton& AddHeaderButton(UINode& InParent, const String& InCaption, float InWidth, const Vec4& InColor,
                                 std::function<void()> InOnClick) {
    UIButton* button = InParent.Add<UIButton>();
    button->Size = { UIValue(InWidth), 1.0_rel };
    button->SetCaption(InCaption);
    EditorStyle::ApplyButtonStyle(*button);
    button->NormalColor = InColor;
    button->Clicked = std::move(InOnClick);
    return *button;
}

InputActionListTab::InputActionListTab() {
    UIVStack* layout = Add<UIVStack>();
    layout->Fill();

    UIQuad* header = layout->Add<UIQuad>();
    header->Size = { 1.0_rel, s_HeaderHeight };
    header->Color = EditorStyle::TabBar;

    UIHStack* buttons = header->Add<UIHStack>();
    buttons->Fill();
    buttons->Padding = UIPadding(6.0f, 4.0f);
    buttons->Gap = 6.0f;

    AddHeaderButton(*buttons, "+ Action", 72.0f, s_AddGreen, [this] {
        if (InputMappingEditorTab* editor = GetEditor()) {
            editor->AddAction();
        }
    });
    m_AddBindingButton = &AddHeaderButton(*buttons, "+ Binding", 80.0f, EditorStyle::Button, [this] {
        UIMenuModel menu;
        BuildAddBindingMenu(menu, GetSelectedAction());
        UIContextMenu::OpenUnder(*m_AddBindingButton, menu);
    });

    UIButton* background = layout->Add<UIButton>();
    background->Size = { 1.0_rel, 1.0_rel };
    background->NormalColor = background->HoverColor = background->PressedColor = EditorStyle::Panel;
    background->Cursor = CursorIcon::Arrow;
    background->Clicked = [this] {
        if (InputMappingEditorTab* editor = GetEditor()) {
            editor->SetSelection(editor->GetMapping());
        }
    };
    background->SecondaryClicked = [this, background](const Vec2& InCursorPos) {
        UIMenuModel menu;
        menu.Item("Add Action", [this] {
            if (InputMappingEditorTab* editor = GetEditor()) {
                editor->AddAction();
            }
        });
        UIContextMenu::OpenAt(*background, InCursorPos, menu);
    };

    UIScrollArea* scroll = background->Add<UIScrollArea>();
    scroll->Fill();

    m_List = scroll->Add<UIVStack>();
    m_List->Anchor = m_List->Pivot = Vec2(0.0f);
    m_List->Position = Vec2(0.0f);
    m_List->Size = { 1.0_rel, 0.0_px };

    m_EmptyLabel = background->Add<UILabel>();
    m_EmptyLabel->Fill();
    m_EmptyLabel->FontSize = EditorStyle::FontSize;
    m_EmptyLabel->Color = EditorStyle::TextDim;
    m_EmptyLabel->HAlign = UIHAlign::Center;
    m_EmptyLabel->VAlign = UIVAlign::Middle;
    m_EmptyLabel->Text = "No actions yet. Add one with + Action.";

    Bind = [this] { Refresh(); };
}

VectorImage* InputActionListTab::GetTabIcon() const {
    return EditorIcons::Outliner();
}

InputMappingEditorTab* InputActionListTab::GetEditor() const {
    return GetMajorTab() ? GetMajorTab()->As<InputMappingEditorTab>() : nullptr;
}

InputAction* InputActionListTab::GetSelectedAction() const {
    InputMappingEditorTab* editor = GetEditor();
    Object* selected = editor ? editor->GetSoleSelection() : nullptr;
    if (InputBinding* binding = Cast<InputBinding>(selected)) {
        return editor->FindOwningAction(binding);
    }
    return Cast<InputAction>(selected);
}

Array<Object*> InputActionListTab::CollectRows() const {
    Array<Object*> rows;
    InputMappingEditorTab* editor = GetEditor();
    InputActionMapping* mapping = editor ? editor->GetMapping() : nullptr;
    if (!mapping) {
        return rows;
    }
    for (const SharedObjectPtr<InputAction>& action : mapping->Actions) {
        rows.Add(action.Get());
        for (const SharedObjectPtr<InputBinding>& binding : action->Bindings) {
            rows.Add(binding.Get());
        }
    }
    return rows;
}

void InputActionListTab::Refresh() {
    const Array<Object*> rows = CollectRows();
    bool stale = rows.Size() != m_Rows.Size();
    for (int32_t i = 0; !stale && i < rows.Size(); i++) {
        stale = m_Rows[i].Get() != rows[i];
    }
    if (stale) {
        RebuildRows(rows);
    }

    m_EmptyLabel->SetEnabled(rows.IsEmpty());
    m_AddBindingButton->SetEnabled(GetSelectedAction() != nullptr);
}

void InputActionListTab::RebuildRows(const Array<Object*>& InRows) {
    while (m_List->HasChildren()) {
        delete m_List->GetChild(0);
    }
    m_Rows.Clear();
    for (Object* target : InRows) {
        AddRow(target);
        m_Rows.Add(target);
    }
    m_List->Size = { 1.0_rel, UIValue(s_RowHeight * (float)InRows.Size()) };
}

void InputActionListTab::AddRow(Object* InTarget) {
    const WeakObjectPtr<Object> target = InTarget;
    const bool isBinding = InTarget->IsA<InputBinding>();
    const float textLeft = isBinding ? s_BindingTextLeft : s_ActionTextLeft;

    UIButton* row = m_List->Add<UIButton>();
    row->Size = { 1.0_rel, s_RowHeight };
    row->PressedColor = EditorStyle::ButtonPressed;
    row->Clicked = [this, target] {
        if (InputMappingEditorTab* editor = GetEditor()) {
            editor->SetSelection(target.Get());
        }
    };
    row->SecondaryClicked = [this, row, target](const Vec2& InCursorPos) {
        InputMappingEditorTab* editor = GetEditor();
        if (!editor || !target.Get()) {
            return;
        }
        editor->SetSelection(target.Get());
        UIMenuModel menu;
        BuildRowMenu(menu, target.Get());
        UIContextMenu::OpenAt(*row, InCursorPos, menu);
    };
    row->Bind = [this, row, target] {
        InputMappingEditorTab* editor = GetEditor();
        const bool selected = editor && editor->IsSelected(target.Get());
        row->NormalColor = selected ? s_SelectedColor : Vec4(0.0f);
        row->HoverColor = selected ? s_SelectedColor : EditorStyle::TabHover;
    };

    if (!isBinding) {
        UISvg* icon = row->Add<UISvg>();
        icon->Anchor = icon->Pivot = Vec2(0.0f, 0.5f);
        icon->Position = Vec2(8.0f, 0.0f);
        icon->Size = Vec2(14.0f, 14.0f);
        icon->Image = EditorIcons::InputMapping();
        icon->Tint = EditorStyle::Text;
    }

    UINode* nameClip = row->Add<UINode>();
    nameClip->Anchor = nameClip->Pivot = Vec2(0.0f, 0.5f);
    nameClip->Position = Vec2(textLeft, 0.0f);
    nameClip->Size = { 1.0_rel - UIValue::Px(textLeft + s_TypeColumnWidth), 1.0_rel };
    nameClip->ClipChildren = true;

    UILabel* name = nameClip->Add<UILabel>();
    name->Fill();
    name->FontSize = EditorStyle::FontSize;
    name->Color = isBinding ? EditorStyle::Text : EditorStyle::TextBright;
    name->VAlign = UIVAlign::Middle;
    name->Bind = [name, target] { name->Text = GetRowName(target.Get()); };

    UILabel* type = row->Add<UILabel>();
    type->Anchor = type->Pivot = Vec2(1.0f, 0.5f);
    type->Position = Vec2(-8.0f, 0.0f);
    type->Size = { UIValue(s_TypeColumnWidth - 8.0f), 1.0_rel };
    type->FontSize = EditorStyle::FontSize;
    type->Color = EditorStyle::TextDim;
    type->HAlign = UIHAlign::Right;
    type->VAlign = UIVAlign::Middle;
    type->Bind = [type, target] { type->Text = GetRowType(target.Get()); };
}

void InputActionListTab::BuildRowMenu(UIMenuModel& OutMenu, Object* InTarget) {
    const WeakObjectPtr<Object> target = InTarget;
    OutMenu.Section(GetRowName(InTarget));
    if (InputAction* action = Cast<InputAction>(InTarget)) {
        const WeakObjectPtr<InputAction> weakAction = action;
        OutMenu.Submenu("Add Binding", [this, weakAction](UIMenuModel& OutSub) {
            BuildAddBindingMenu(OutSub, weakAction.Get());
        });
        OutMenu.Separator();
    }
    OutMenu.Item("Delete", [this, target] {
        if (InputMappingEditorTab* editor = GetEditor()) {
            editor->Delete(target.Get());
        }
    }).Shortcut("Del");
}

void InputActionListTab::BuildAddBindingMenu(UIMenuModel& OutMenu, InputAction* InAction) {
    if (!InAction) {
        return;
    }
    const WeakObjectPtr<InputAction> action = InAction;
    for (const Class& bindingClass : InputMappingEditorTab::GetBindingClassesFor(InAction->Type)) {
        OutMenu.Item(InputMappingEditorTab::GetBindingLabel(bindingClass), [this, action, bindingClass] {
            InputMappingEditorTab* editor = GetEditor();
            if (editor && action.Get()) {
                editor->AddBinding(action.Get(), bindingClass);
            }
        });
    }
}

void InputActionListTab::OnUIUpdate(const UIFrameContext& InContext) {
    (void)InContext;
    KeyboardDevice* keyboard = KeyboardDevice::Instance();
    InputMappingEditorTab* editor = GetEditor();
    if (!keyboard || !editor || !AcceptsShortcuts()) {
        return;
    }
    if (keyboard->IsDown(KeyCode::Delete) || keyboard->IsDown(KeyCode::Backspace)) {
        editor->Delete(editor->GetSoleSelection());
    }
}

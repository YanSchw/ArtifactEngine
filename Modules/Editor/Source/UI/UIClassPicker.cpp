#include "UIClassPicker.h"
#include "EditorStyle.h"
#include "EditorIcons.h"
#include "GameFramework/Component.h"
#include "GameFramework/UIButton.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UIScrollArea.h"
#include "GameFramework/UISvg.h"
#include "GameFramework/UITextArea.h"
#include "GameFramework/UIVStack.h"
#include <algorithm>
#include <cctype>
#include <chrono>

static constexpr float s_SearchHeight = 26.0f;
static constexpr float s_RowHeight = 22.0f;
static constexpr float s_IndentWidth = 14.0f;
static constexpr float s_IconSize = 14.0f;
static constexpr double s_DoubleClickSeconds = 0.35;

static String Lowered(const String& InText) {
    String lowered = InText;
    for (char& character : lowered) {
        character = (char)std::tolower((unsigned char)character);
    }
    return lowered;
}

static double NowSeconds() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

UIClassPicker::UIClassPicker() {
    UIVStack* column = Add<UIVStack>();
    column->Fill();
    column->Padding = UIPadding(4.0f);
    column->Gap = 4.0f;

    UINode* searchRow = column->Add<UINode>();
    searchRow->Size = { 1.0_rel, UIValue(s_SearchHeight) };
    m_Search = searchRow->Add<UITextArea>();
    m_Search->Fill();
    m_Search->Padding = UIPadding(24.0f, 0.0f, 6.0f, 0.0f);
    m_Search->SingleLine = true;
    m_Search->Placeholder = "Search classes...";
    m_Search->FontSize = EditorStyle::FontSize;
    m_Search->TextColor = EditorStyle::Text;
    m_Search->PlaceholderColor = EditorStyle::TextDim;
    m_Search->BackgroundColor = EditorStyle::Panel;
    m_Search->FocusedBorderColor = EditorStyle::Accent;
    m_Search->CornerRadius = EditorStyle::ButtonRadius;
    m_Search->TextChanged = [this](const String& InText) {
        m_Filter = Lowered(InText);
        m_Dirty = true;
    };
    UISvg* searchIcon = searchRow->Add<UISvg>();
    searchIcon->Anchor = searchIcon->Pivot = Vec2(0.0f, 0.5f);
    searchIcon->Position = Vec2(8.0f, 0.0f);
    searchIcon->Size = Vec2(11.0f);
    searchIcon->Image = EditorIcons::Search();
    searchIcon->Tint = EditorStyle::TextDim;

    UIScrollArea* scroll = column->Add<UIScrollArea>();
    scroll->Size = { 1.0_rel, 1.0_rel };
    m_Rows = scroll->Add<UIVStack>();
    m_Rows->Fill();
}

bool UIClassPicker::IsPlaceableNodeClass(const Class& InClass) {
    return !InClass.IsSubclassOf(Component::StaticClass()) && InClass.GetModuleName() != "Editor";
}

void UIClassPicker::FocusSearch() {
    m_Search->RequestFocus();
}

void UIClassPicker::CollectHierarchy() {
    m_HierarchyBuilt = true;
    for (const Class& candidate : Class::GetSubclassesOf(RootClass)) {
        if (Filter && !Filter(candidate)) {
            continue;
        }
        m_Pickable[candidate.Name] = candidate.IsDefaultConstructable();
        if (candidate != RootClass) {
            m_Children[candidate.GetParentClass().Name].Add(candidate);
        }
    }
    for (auto& [parent, children] : m_Children) {
        std::sort(children.begin(), children.end(), [](const Class& InA, const Class& InB) { return InA.Name < InB.Name; });
    }

    for (Class current = Selected; current != Class::None && current != RootClass; current = current.GetParentClass()) {
        m_Expanded.Add(current.GetParentClass().Name);
    }
    m_Expanded.Add(RootClass.Name);
    m_Expanded.Add(Selected.Name);
}

bool UIClassPicker::IsVisible(const Class& InClass) const {
    if (!m_Pickable.ContainsKey(InClass.Name)) {
        return false;
    }
    if (m_Pickable.At(InClass.Name) && (m_Filter.empty() || Lowered(InClass.Name).find(m_Filter) != String::npos)) {
        return true;
    }
    if (m_Children.ContainsKey(InClass.Name)) {
        for (const Class& child : m_Children.At(InClass.Name)) {
            if (IsVisible(child)) {
                return true;
            }
        }
    }
    return false;
}

void UIClassPicker::OnBind() {
    UINode::OnBind();
    if (!m_HierarchyBuilt) {
        CollectHierarchy();
    }
    if (m_Dirty) {
        m_Dirty = false;
        Rebuild();
    }
}

void UIClassPicker::Rebuild() {
    while (m_Rows->HasChildren()) {
        delete m_Rows->GetChild(0);
    }
    if (IsVisible(RootClass)) {
        AddRow(RootClass, 0);
    }
}

void UIClassPicker::AddRow(const Class& InClass, int32_t InDepth) {
    Array<Class> children;
    if (m_Children.ContainsKey(InClass.Name)) {
        for (const Class& child : m_Children.At(InClass.Name)) {
            if (IsVisible(child)) {
                children.Add(child);
            }
        }
    }
    const bool pickable = m_Pickable.At(InClass.Name);
    const bool expanded = !m_Filter.empty() || m_Expanded.Contains(InClass.Name);
    const bool matches = !m_Filter.empty() && Lowered(InClass.Name).find(m_Filter) != String::npos;
    const float indent = 4.0f + (float)InDepth * s_IndentWidth;

    UIButton* row = m_Rows->Add<UIButton>();
    row->Size = { 1.0_rel, UIValue(s_RowHeight) };
    row->CornerRadius = Vec4(EditorStyle::ButtonRadius - 1.0f);
    row->PressedColor = EditorStyle::ButtonPressed;
    row->Clicked = [this, InClass] { OnRowClicked(InClass); };
    row->Bind = [this, row, InClass] {
        const bool selected = Selected == InClass;
        row->NormalColor = selected ? EditorStyle::Accent : Vec4(0.0f);
        row->HoverColor = selected ? EditorStyle::Accent : EditorStyle::TabHover;
    };

    if (!children.IsEmpty()) {
        UIButton* toggle = row->Add<UIButton>();
        toggle->Anchor = toggle->Pivot = Vec2(0.0f, 0.5f);
        toggle->Position = Vec2(indent - 2.0f, 0.0f);
        toggle->Size = Vec2(16.0f, s_RowHeight);
        toggle->NormalColor = toggle->HoverColor = toggle->PressedColor = Vec4(0.0f);
        toggle->Clicked = [this, InClass] {
            if (m_Expanded.Contains(InClass.Name)) {
                m_Expanded.Remove(InClass.Name);
            } else {
                m_Expanded.Add(InClass.Name);
            }
            m_Dirty = true;
        };
        UISvg* arrow = toggle->Add<UISvg>();
        arrow->Center(Vec2(9.0f));
        arrow->Image = expanded ? EditorIcons::ArrowDown() : EditorIcons::ArrowRight();
        arrow->Tint = EditorStyle::TextDim;
    }

    UISvg* icon = row->Add<UISvg>();
    icon->Anchor = icon->Pivot = Vec2(0.0f, 0.5f);
    icon->Position = Vec2(indent + 16.0f, 0.0f);
    icon->Size = Vec2(s_IconSize);
    icon->Image = EditorIcons::GetNodeIcon(InClass);
    icon->Tint = pickable ? EditorStyle::Text : EditorStyle::TextDim;

    UILabel* label = row->Add<UILabel>();
    label->Anchor = label->Pivot = Vec2(0.0f, 0.5f);
    label->Position = Vec2(indent + 16.0f + s_IconSize + 6.0f, 0.0f);
    label->Size = { 1.0_rel - UIValue::Px(indent + 42.0f), 1.0_rel };
    label->FontSize = EditorStyle::FontSize;
    label->VAlign = UIVAlign::Middle;
    label->Text = InClass.GetDisplayName();
    label->Bind = [this, label, InClass, pickable, matches] {
        label->Color = Selected == InClass ? EditorStyle::TextBright
                     : matches ? EditorStyle::AccentBright
                     : pickable ? EditorStyle::Text : EditorStyle::TextDim;
    };

    if (expanded) {
        for (const Class& child : children) {
            AddRow(child, InDepth + 1);
        }
    }
}

void UIClassPicker::OnRowClicked(const Class& InClass) {
    if (!m_Pickable.At(InClass.Name)) {
        if (m_Expanded.Contains(InClass.Name)) {
            m_Expanded.Remove(InClass.Name);
        } else {
            m_Expanded.Add(InClass.Name);
        }
        m_Dirty = true;
        return;
    }

    const double now = NowSeconds();
    const bool doubleClick = m_LastClicked == InClass && now - m_LastClickTime < s_DoubleClickSeconds;
    m_LastClicked = InClass;
    m_LastClickTime = now;
    Selected = InClass;
    if (doubleClick && Confirmed) {
        Confirmed(InClass);
    }
}

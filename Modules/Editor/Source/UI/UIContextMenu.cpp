#include "UIContextMenu.h"
#include "EditorStyle.h"
#include "EditorIcons.h"
#include "GameFramework/UICanvas.h"
#include "GameFramework/UIVStack.h"
#include "GameFramework/UIScrollArea.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UISvg.h"
#include "GameFramework/UITextArea.h"
#include "InputSystem/KeyboardDevice.h"
#include "InputSystem/KeyCodes.h"
#include "Assets/Font.h"
#include "Rendering/UIDrawList.h"
#include <algorithm>
#include <cctype>

static constexpr float s_SidePadding = 10.0f;
static constexpr float s_IconColumn = 32.0f;
static constexpr float s_ArrowColumn = 22.0f;
static constexpr float s_ShortcutGap = 30.0f;
static constexpr float s_PanelPadding = 4.0f;
static constexpr float s_CornerRadius = 5.0f;
static constexpr float s_SearchHeight = 28.0f;
static constexpr float s_MinPanelWidth = 150.0f;
static constexpr float s_MaxPanelWidth = 440.0f;
static constexpr float s_ScreenMargin = 4.0f;
static constexpr float s_SubmenuDelay = 0.16f;
static constexpr float s_TooltipDelay = 0.6f;

static const Vec4 s_DisabledText = HexColor(0x5C5C5C);
static const Vec4 s_SeparatorLine = Vec4(1.0f, 1.0f, 1.0f, 0.09f);
static const Vec4 s_Shadow = Vec4(0.0f, 0.0f, 0.0f, 0.35f);

static float SectionFontSize() { return EditorStyle::FontSize - 1.0f; }
static float ShortcutFontSize() { return EditorStyle::FontSize - 1.0f; }

static float MeasureText(const String& InText, float InFontSize) {
    Font* font = UINode::GetDefaultFont();
    return font ? font->GetTextWidth(InText, InFontSize) : (float)InText.size() * InFontSize * 0.5f;
}

static String ToLower(const String& InText) {
    String lowered = InText;
    for (char& character : lowered) {
        character = (char)std::tolower((unsigned char)character);
    }
    return lowered;
}

/* ---------------------------------- UIMenuRow ---------------------------------- */

UIMenuRow::UIMenuRow() {
    Size = { 1.0_rel, UIValue(ActionHeight) };

    m_Icon = Add<UISvg>();
    m_Icon->Anchor = m_Icon->Pivot = Vec2(0.0f, 0.5f);
    m_Icon->Position = Vec2(s_SidePadding, 0.0f);
    m_Icon->Size = Vec2(14.0f);
    m_Icon->SetEnabled(false);

    m_Label = Add<UILabel>();
    m_Label->Anchor = m_Label->Pivot = Vec2(0.0f, 0.5f);
    m_Label->FontSize = EditorStyle::FontSize;
    m_Label->VAlign = UIVAlign::Middle;

    m_Shortcut = Add<UILabel>();
    m_Shortcut->Anchor = m_Shortcut->Pivot = Vec2(1.0f, 0.5f);
    m_Shortcut->Size = { 160.0_px, 1.0_rel };
    m_Shortcut->FontSize = ShortcutFontSize();
    m_Shortcut->HAlign = UIHAlign::Right;
    m_Shortcut->VAlign = UIVAlign::Middle;
    m_Shortcut->SetEnabled(false);

    m_Arrow = Add<UISvg>();
    m_Arrow->Anchor = m_Arrow->Pivot = Vec2(1.0f, 0.5f);
    m_Arrow->Position = Vec2(-8.0f, 0.0f);
    m_Arrow->Size = Vec2(9.0f);
    m_Arrow->Image = EditorIcons::ArrowRight();
    m_Arrow->SetEnabled(false);
}

float UIMenuRow::HeightFor(const UIMenuEntry& InEntry) {
    switch (InEntry.Type) {
        case UIMenuEntry::Kind::Section: return SectionHeight;
        case UIMenuEntry::Kind::Separator: return SeparatorHeight;
        default: return ActionHeight;
    }
}

void UIMenuRow::SetEntry(const UIMenuEntry& InEntry, float InLabelLeft, bool InArrowColumn) {
    m_Entry = InEntry;
    const bool action = (m_Entry.Type == UIMenuEntry::Kind::Action);
    const float rightInset = InArrowColumn ? s_ArrowColumn : s_SidePadding;

    Size = { 1.0_rel, UIValue(HeightFor(m_Entry)) };
    Interactable = action;
    Cursor = (action && m_Entry.Enabled) ? CursorIcon::Hand : CursorIcon::Arrow;

    m_Icon->SetEnabled(m_Entry.Icon != nullptr);
    m_Icon->Image = m_Entry.Icon;

    m_Label->SetEnabled(m_Entry.Type != UIMenuEntry::Kind::Separator);
    m_Label->Text = m_Entry.Label;
    m_Label->FontSize = action ? EditorStyle::FontSize : SectionFontSize();
    m_Label->Position = Vec2(action ? InLabelLeft : s_SidePadding, 0.0f);
    m_Label->Size = { 1.0_rel - UIValue::Px(InLabelLeft + rightInset), 1.0_rel };

    m_Shortcut->SetEnabled(!m_Entry.Shortcut.empty());
    m_Shortcut->Text = m_Entry.Shortcut;
    m_Shortcut->Position = Vec2(-rightInset, 0.0f);

    m_Arrow->SetEnabled(action && (bool)m_Entry.Submenu);

    ApplyColors(false);
}

bool UIMenuRow::IsHighlighted() const {
    return Owner && Owner->GetHighlight() == Index
        && m_Entry.Type == UIMenuEntry::Kind::Action && m_Entry.Enabled;
}

void UIMenuRow::ApplyColors(bool InHighlighted) {
    Vec4 text = EditorStyle::Text;
    if (m_Entry.Type != UIMenuEntry::Kind::Action) {
        text = EditorStyle::TextDim;
    } else if (!m_Entry.Enabled) {
        text = s_DisabledText;
    } else if (InHighlighted) {
        text = EditorStyle::TextBright;
    }

    m_Label->Color = text;
    m_Icon->Tint = text;
    m_Arrow->Tint = text;
    m_Shortcut->Color = InHighlighted ? EditorStyle::TextBright
                                      : (m_Entry.Enabled ? EditorStyle::TextDim : s_DisabledText);
}

void UIMenuRow::Paint(UIDrawList& OutDrawList) {
    if (m_Entry.Type == UIMenuEntry::Kind::Separator) {
        const UIRectF line(Vec2(m_Geometry.Min().x + s_SidePadding, m_Geometry.Center().y),
                           Vec2(std::max(0.0f, m_Geometry.Size.x - 2.0f * s_SidePadding), 1.0f));
        OutDrawList.AddRect(line, s_SeparatorLine, m_WorldMatrix);
        return;
    }

    if (m_Entry.Type == UIMenuEntry::Kind::Section) {
        const float left = m_Geometry.Min().x + s_SidePadding + MeasureText(m_Entry.Label, SectionFontSize()) + 8.0f;
        const float right = m_Geometry.Max().x - s_SidePadding;
        if (right > left) {
            OutDrawList.AddRect(UIRectF(Vec2(left, m_Geometry.Center().y), Vec2(right - left, 1.0f)),
                                s_SeparatorLine, m_WorldMatrix);
        }
        return;
    }

    if (IsHighlighted()) {
        OutDrawList.AddRoundedRect(m_Geometry.Deflate(UIPadding(3.0f, 0.0f)), EditorStyle::Accent, 3.0f, m_WorldMatrix);
    }

    if (m_Entry.Checked && !m_Entry.Icon) {
        const Vec2 center(m_Geometry.Min().x + s_SidePadding + 7.0f, m_Geometry.Center().y);
        const Vec4 tick = m_Entry.Enabled ? EditorStyle::AccentBright : s_DisabledText;
        OutDrawList.AddLine(center + Vec2(-5.0f, 0.0f), center + Vec2(-1.5f, 3.5f), 2.0f, tick, m_WorldMatrix);
        OutDrawList.AddLine(center + Vec2(-1.5f, 3.5f), center + Vec2(5.0f, -4.0f), 2.0f, tick, m_WorldMatrix);
    }
}

void UIMenuRow::OnClick() {
    if (UIMenuPanel* owner = Owner) {
        owner->ActivateRow(Index);  // may destroy this row
    }
}

void UIMenuRow::OnUIUpdate(const UIFrameContext& InContext) {
    ApplyColors(IsHighlighted());

    if (!IsHovered()) {
        m_HoverSeconds = 0.0f;
        return;
    }
    m_HoverSeconds += InContext.DeltaTime;
    if (m_HoverSeconds < s_TooltipDelay || m_Entry.Tooltip.empty()) {
        return;
    }
    if (UIContextMenu* menu = Owner ? Owner->GetMenu() : nullptr) {
        menu->RequestTooltip(m_Entry.Tooltip, m_Geometry);
    }
}

/* --------------------------------- UIMenuPanel --------------------------------- */

UIMenuPanel::UIMenuPanel() {
    Anchor = Pivot = Vec2(0.0f);
    Size = Vec2(s_MinPanelWidth, 0.0f);
    Interactable = true;  // the panel chrome swallows clicks instead of dismissing the menu
}

void UIMenuPanel::Build(UIContextMenu& InMenu, UIMenuPanel* InParentPanel, const UIMenuModel& InModel) {
    m_Menu = &InMenu;
    m_ParentPanel = InParentPanel;
    m_Model = InModel;

    UIVStack* column = Add<UIVStack>();
    column->Fill();
    column->Padding = UIPadding(1.0f, s_PanelPadding + 1.0f);

    if (m_Model.IsSearchable()) {
        UINode* searchRow = column->Add<UINode>();
        searchRow->Size = { 1.0_rel, UIValue(s_SearchHeight) };

        m_Search = searchRow->Add<UITextArea>();
        m_Search->Center({ 1.0_rel - 12.0_px, 22.0f });
        m_Search->SingleLine = true;
        m_Search->Placeholder = m_Model.GetSearchPlaceholder();
        m_Search->FontSize = EditorStyle::FontSize;
        m_Search->TextColor = EditorStyle::Text;
        m_Search->PlaceholderColor = EditorStyle::TextDim;
        m_Search->BackgroundColor = EditorStyle::PanelDark;
        m_Search->FocusedBorderColor = EditorStyle::Accent;
        m_Search->CornerRadius = 3.0f;
        m_Search->Padding = UIPadding(6.0f, 0.0f);
        m_Search->RequestFocus();
    }

    m_Scroll = column->Add<UIScrollArea>();
    m_Scroll->Size = { 1.0_rel, 1.0_rel };

    UIVStack* list = m_Scroll->Add<UIVStack>();
    list->Anchor = list->Pivot = Vec2(0.0f);
    list->Position = Vec2(0.0f);
    list->Size = { 1.0_rel, 0.0_px };  // width fills; the rows stack past it and drive the scroll
    m_List = list;

    RebuildRows();
}

Array<UIMenuRow*> UIMenuPanel::GetRows() const {
    Array<UIMenuRow*> rows;
    if (!m_List) {
        return rows;
    }
    for (uint32_t i = 0; i < m_List->GetChildCount(); i++) {
        if (UIMenuRow* row = m_List->GetChild((int)i)->As<UIMenuRow>()) {
            rows.Add(row);
        }
    }
    return rows;
}

UIMenuRow* UIMenuPanel::FindRow(int32_t InIndex) const {
    for (UIMenuRow* row : GetRows()) {
        if (row->Index == InIndex) {
            return row;
        }
    }
    return nullptr;
}

UIRectF UIMenuPanel::GetCanvasRect() const {
    UICanvas* canvas = GetCanvas();
    return canvas ? canvas->GetGeometry() : UIRectF();
}

void UIMenuPanel::RebuildRows() {
    CloseSubmenu();
    while (m_List->HasChildren()) {
        delete m_List->GetChild(0);
    }
    m_Highlight = -1;
    m_HoverRow = -1;

    const Array<UIMenuEntry>& entries = m_Model.GetEntries();
    Array<int32_t> visible;
    for (int32_t i = 0; i < entries.Size(); i++) {
        const UIMenuEntry& entry = entries[i];
        if (m_Filter.empty()) {
            visible.Add(i);
        } else if (entry.Type == UIMenuEntry::Kind::Action && ToLower(entry.Label).find(m_Filter) != String::npos) {
            visible.Add(i);
        }
    }

    bool iconColumn = false;
    bool arrowColumn = false;
    for (int32_t index : visible) {
        iconColumn = iconColumn || entries[index].Icon || entries[index].Checked;
        arrowColumn = arrowColumn || (bool)entries[index].Submenu;
    }
    const float labelLeft = iconColumn ? s_IconColumn : s_SidePadding;
    const float rightInset = arrowColumn ? s_ArrowColumn : s_SidePadding;

    float width = std::max(m_Model.GetMinWidth(), s_MinPanelWidth);
    float height = 0.0f;
    for (int32_t index : visible) {
        const UIMenuEntry& entry = entries[index];
        const bool action = (entry.Type == UIMenuEntry::Kind::Action);
        float rowWidth = (action ? labelLeft : s_SidePadding) + rightInset
                       + MeasureText(entry.Label, action ? EditorStyle::FontSize : SectionFontSize());
        if (!entry.Shortcut.empty()) {
            rowWidth += s_ShortcutGap + MeasureText(entry.Shortcut, ShortcutFontSize());
        }
        width = std::max(width, rowWidth);
        height += UIMenuRow::HeightFor(entry);
    }

    for (int32_t index : visible) {
        UIMenuRow* row = m_List->Add<UIMenuRow>();
        row->Owner = this;
        row->Index = index;
        row->SetEntry(entries[index], labelLeft, arrowColumn);
    }

    const float chrome = 2.0f + 2.0f * s_PanelPadding + (m_Search ? s_SearchHeight : 0.0f);
    const float maxHeight = std::max(120.0f, GetCanvasRect().Size.y - 2.0f * s_ScreenMargin);
    Size = Vec2(std::min(width, s_MaxPanelWidth), std::min(height + chrome, maxHeight));
    PlaceAt(m_DesiredTopLeft);
}

void UIMenuPanel::PlaceAt(const Vec2& InTopLeft) {
    m_DesiredTopLeft = InTopLeft;

    const UIRectF canvas = GetCanvasRect();
    const Vec2 size(Size.X.Pixels, Size.Y.Pixels);
    Vec2 position = InTopLeft;
    if (position.x + size.x > canvas.Max().x - s_ScreenMargin) {
        position.x = InTopLeft.x - size.x;
    }
    if (position.y + size.y > canvas.Max().y - s_ScreenMargin) {
        position.y = canvas.Max().y - s_ScreenMargin - size.y;
    }
    position = Vec2(std::max(position.x, canvas.Min().x + s_ScreenMargin),
                    std::max(position.y, canvas.Min().y + s_ScreenMargin));
    Position = position - canvas.Min();
}

void UIMenuPanel::PlaceBeside(const UIRectF& InRowRect) {
    const UIRectF canvas = GetCanvasRect();
    const float width = Size.X.Pixels;
    float x = InRowRect.Max().x + 2.0f;
    if (x + width > canvas.Max().x - s_ScreenMargin) {
        x = InRowRect.Min().x - width - 2.0f;
    }
    PlaceAt(Vec2(x, InRowRect.Min().y - s_PanelPadding - 1.0f));
}

void UIMenuPanel::PlaceUnder(const UIRectF& InAnchorRect) {
    const UIRectF canvas = GetCanvasRect();
    const float height = Size.Y.Pixels;
    float y = InAnchorRect.Max().y + 2.0f;
    if (y + height > canvas.Max().y - s_ScreenMargin) {
        y = InAnchorRect.Min().y - height - 2.0f;
    }
    PlaceAt(Vec2(InAnchorRect.Min().x, y));
}

void UIMenuPanel::ActivateRow(int32_t InIndex) {
    UIContextMenu* menu = m_Menu.Get();
    if (!menu || menu->IsClosing() || InIndex < 0 || InIndex >= m_Model.GetEntries().Size()) {
        return;
    }
    const UIMenuEntry& entry = m_Model.GetEntries()[InIndex];
    if (entry.Type != UIMenuEntry::Kind::Action || !entry.Enabled) {
        return;
    }
    if (entry.Submenu) {
        OpenSubmenu(InIndex);
        return;
    }

    // The action may open a menu of its own, which destroys this panel, so touch nothing after it.
    const std::function<void()> activated = entry.Activated;
    menu->Close();
    if (activated) {
        activated();
    }
}

void UIMenuPanel::OpenSubmenu(int32_t InIndex) {
    if (m_OpenRow == InIndex && m_Submenu.Get()) {
        return;
    }
    CloseSubmenu();

    UIContextMenu* menu = m_Menu.Get();
    UIMenuRow* row = FindRow(InIndex);
    if (!menu || !row) {
        return;
    }
    const UIMenuEntry& entry = m_Model.GetEntries()[InIndex];
    if (!entry.Submenu || !entry.Enabled) {
        return;
    }

    UIMenuModel model;
    entry.Submenu(model);
    if (model.IsEmpty()) {
        return;
    }

    UIMenuPanel* panel = menu->Add<UIMenuPanel>();
    panel->Build(*menu, this, model);
    panel->PlaceBeside(row->GetGeometry());
    m_Submenu = panel;
    m_OpenRow = InIndex;
}

void UIMenuPanel::CloseSubmenu() {
    if (UIMenuPanel* submenu = m_Submenu.Get()) {
        submenu->CloseSubmenu();
        if (UICanvas* canvas = GetCanvas()) {
            canvas->DestroyDeferred(submenu);
        }
    }
    m_Submenu = nullptr;
    m_OpenRow = -1;
}

void UIMenuPanel::MoveHighlight(int32_t InDelta) {
    Array<int32_t> selectable;
    for (UIMenuRow* row : GetRows()) {
        const UIMenuEntry& entry = row->GetEntry();
        if (entry.Type == UIMenuEntry::Kind::Action && entry.Enabled) {
            selectable.Add(row->Index);
        }
    }
    if (selectable.IsEmpty()) {
        return;
    }
    const int32_t current = selectable.IndexOf(m_Highlight);
    const int32_t next = (current < 0) ? (InDelta > 0 ? 0 : selectable.Size() - 1)
                                       : (current + InDelta + selectable.Size()) % selectable.Size();
    CloseSubmenu();
    m_Highlight = selectable[next];
}

void UIMenuPanel::ActivateHighlight() {
    ActivateRow(m_Highlight);
}

void UIMenuPanel::OpenHighlightedSubmenu() {
    OpenSubmenu(m_Highlight);
    if (UIMenuPanel* submenu = m_Submenu.Get()) {
        submenu->MoveHighlight(1);
    }
}

void UIMenuPanel::Paint(UIDrawList& OutDrawList) {
    const UIRectF shadow(m_Geometry.Position + Vec2(1.0f, 3.0f), m_Geometry.Size);
    OutDrawList.AddRoundedRect(shadow, s_Shadow, s_CornerRadius, m_WorldMatrix);
    OutDrawList.AddRoundedRect(m_Geometry, EditorStyle::Border, s_CornerRadius, m_WorldMatrix);
    OutDrawList.AddRoundedRect(m_Geometry.Deflate(UIPadding(1.0f)), EditorStyle::Panel, s_CornerRadius - 1.0f, m_WorldMatrix);
}

void UIMenuPanel::OnBind() {
    UINode::OnBind();

    if (m_Search) {
        const String filter = ToLower(m_Search->Text);
        if (filter != m_Filter) {
            m_Filter = filter;
            m_RowsDirty = true;
        }
    }
    if (m_RowsDirty) {
        m_RowsDirty = false;
        RebuildRows();
    }
}

void UIMenuPanel::OnUIUpdate(const UIFrameContext& InContext) {
    int32_t hovered = -1;
    for (UIMenuRow* row : GetRows()) {
        if (row->IsHovered()) {
            hovered = row->Index;
            break;
        }
    }

    if (hovered != m_HoverRow) {
        m_HoverRow = hovered;
        m_HoverSeconds = 0.0f;
    } else {
        m_HoverSeconds += InContext.DeltaTime;
    }

    if (hovered < 0) {
        return;
    }
    m_Highlight = hovered;
    if (m_HoverSeconds < s_SubmenuDelay || hovered == m_OpenRow) {
        return;
    }
    if (m_Model.GetEntries()[hovered].Submenu) {
        OpenSubmenu(hovered);
    } else {
        CloseSubmenu();
    }
}

/* -------------------------------- UIContextMenu -------------------------------- */

UIContextMenu::UIContextMenu() {
    Fill();
    Interactable = true;
}

UIContextMenu* UIContextMenu::Spawn(UINode& InOwner, const UIMenuModel& InModel, UIMenuPanel*& OutPanel) {
    OutPanel = nullptr;
    UICanvas* canvas = InOwner.GetCanvas();
    if (!canvas || InModel.IsEmpty()) {
        return nullptr;
    }
    CloseAll(InOwner);

    UIContextMenu* menu = canvas->Add<UIContextMenu>();
    OutPanel = menu->Add<UIMenuPanel>();
    OutPanel->Build(*menu, nullptr, InModel);
    return menu;
}

UIContextMenu* UIContextMenu::OpenAt(UINode& InOwner, const Vec2& InScreenPos, const UIMenuModel& InModel) {
    UIMenuPanel* panel = nullptr;
    UIContextMenu* menu = Spawn(InOwner, InModel, panel);
    if (panel) {
        panel->PlaceAt(InScreenPos);
    }
    return menu;
}

UIContextMenu* UIContextMenu::OpenUnder(UINode& InAnchor, const UIMenuModel& InModel) {
    UIMenuPanel* panel = nullptr;
    UIContextMenu* menu = Spawn(InAnchor, InModel, panel);
    if (panel) {
        panel->PlaceUnder(InAnchor.GetGeometry());
    }
    return menu;
}

void UIContextMenu::CloseAll(UINode& InOwner) {
    UICanvas* canvas = InOwner.GetCanvas();
    if (!canvas) {
        return;
    }
    for (int32_t i = (int32_t)canvas->GetChildCount() - 1; i >= 0; i--) {
        if (UIContextMenu* menu = canvas->GetChild(i)->As<UIContextMenu>()) {
            menu->Close();
        }
    }
}

bool UIContextMenu::IsOpen(UINode& InOwner) {
    UICanvas* canvas = InOwner.GetCanvas();
    if (!canvas) {
        return false;
    }
    for (int32_t i = (int32_t)canvas->GetChildCount() - 1; i >= 0; i--) {
        UIContextMenu* menu = canvas->GetChild(i)->As<UIContextMenu>();
        if (menu && !menu->IsClosing()) {
            return true;
        }
    }
    return false;
}

void UIContextMenu::Close() {
    if (m_Closing) {
        return;
    }
    m_Closing = true;
    if (UICanvas* canvas = GetCanvas()) {
        canvas->DestroyDeferred(this);
    } else {
        SetEnabled(false);
    }
}

UIMenuPanel* UIContextMenu::GetRootPanel() const {
    for (uint32_t i = 0; i < GetChildCount(); i++) {
        if (UIMenuPanel* panel = GetChild((int)i)->As<UIMenuPanel>()) {
            return panel;
        }
    }
    return nullptr;
}

UIMenuPanel* UIContextMenu::GetDeepestPanel() const {
    UIMenuPanel* panel = GetRootPanel();
    while (panel && panel->GetSubmenu()) {
        panel = panel->GetSubmenu();
    }
    return panel;
}

void UIContextMenu::RequestTooltip(const String& InText, const UIRectF& InRowRect) {
    m_TooltipText = InText;
    m_TooltipAnchor = InRowRect;
}

void UIContextMenu::OnPressed(const Vec2& InCursorPos) {
    (void)InCursorPos;
    Close();
}

bool UIContextMenu::OnSecondaryClick(const Vec2& InCursorPos) {
    (void)InCursorPos;
    Close();  // a right-click anywhere but the panels dismisses, like a left one
    return true;
}

void UIContextMenu::OnUIUpdate(const UIFrameContext& InContext) {
    (void)InContext;
    m_TooltipText.clear();  // the hovered row asks for it again later in this same update pass
    HandleKeys();
}

void UIContextMenu::HandleKeys() {
    KeyboardDevice* keyboard = KeyboardDevice::Instance();
    UIMenuPanel* panel = GetDeepestPanel();
    if (!keyboard || !panel) {
        return;
    }

    if (keyboard->IsDown(KeyCode::Escape)) {
        if (UIMenuPanel* parent = panel->GetParentPanel()) {
            parent->CloseSubmenu();
        } else {
            Close();
        }
        return;
    }
    if (keyboard->IsDown(KeyCode::Enter)) {
        panel->ActivateHighlight();
        return;
    }
    if (keyboard->IsDown(KeyCode::Down)) {
        panel->MoveHighlight(1);
    }
    if (keyboard->IsDown(KeyCode::Up)) {
        panel->MoveHighlight(-1);
    }
    if (keyboard->IsDown(KeyCode::Right)) {
        panel->OpenHighlightedSubmenu();
    }
    if (keyboard->IsDown(KeyCode::Left)) {
        if (UIMenuPanel* parent = panel->GetParentPanel()) {
            parent->CloseSubmenu();
        }
    }
}

void UIContextMenu::PaintOverlay(UIDrawList& OutDrawList) {
    Font* font = GetDefaultFont();
    if (m_TooltipText.empty() || !font) {
        return;
    }

    const Vec2 padding(8.0f, 5.0f);
    const Vec2 size = font->MeasureText(m_TooltipText, EditorStyle::FontSize) + padding * 2.0f;
    Vec2 position(m_TooltipAnchor.Min().x + 16.0f, m_TooltipAnchor.Max().y + 6.0f);
    position.x = std::min(position.x, m_Geometry.Max().x - s_ScreenMargin - size.x);
    if (position.y + size.y > m_Geometry.Max().y - s_ScreenMargin) {
        position.y = m_TooltipAnchor.Min().y - size.y - 6.0f;
    }
    position = Vec2(std::max(position.x, m_Geometry.Min().x + s_ScreenMargin),
                    std::max(position.y, m_Geometry.Min().y + s_ScreenMargin));

    const UIRectF rect(position, size);
    OutDrawList.AddRoundedRect(rect, EditorStyle::Border, 4.0f, m_WorldMatrix);
    OutDrawList.AddRoundedRect(rect.Deflate(UIPadding(1.0f)), EditorStyle::ToolBar, 3.0f, m_WorldMatrix);
    OutDrawList.AddText(font, m_TooltipText, position + padding, EditorStyle::FontSize, EditorStyle::Text, m_WorldMatrix);
}

#include "OutlinerRow.h"
#include "OutlinerTab.h"
#include "UI/EditorIcons.h"
#include "UI/EditorStyle.h"
#include "GameFramework/UISvg.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UITextArea.h"
#include "GameFramework/Node.h"
#include "InputSystem/KeyboardDevice.h"
#include "Rendering/UIDrawList.h"

static const Vec4 s_SelectedColor = HexColor(0x0F5A9E);
static const Vec4 s_DropColor = HexColor(0x26BBFF, 0.35f);
static const Vec4 s_AltRowColor = Vec4(1.0f, 1.0f, 1.0f, 0.03f);
static const Vec4 s_DisabledText = HexColor(0x5C5C5C);

OutlinerRow::OutlinerRow() {
    const auto leftAligned = [](UINode& InNode) {
        InNode.Anchor = InNode.Pivot = Vec2(0.0f, 0.5f);
    };

    m_Eye = Add<UISvg>();
    leftAligned(*m_Eye);
    m_Eye->Position = Vec2(3.0f, 0.0f);
    m_Eye->Size = Vec2(14.0f, 14.0f);

    m_Arrow = Add<UISvg>();
    leftAligned(*m_Arrow);
    m_Arrow->Size = Vec2(12.0f, 12.0f);
    m_Arrow->Tint = EditorStyle::TextDim;

    m_Icon = Add<UISvg>();
    leftAligned(*m_Icon);
    m_Icon->Size = Vec2(15.0f, 15.0f);
    m_Icon->Tint = EditorStyle::Text;

    m_Label = Add<UILabel>();
    leftAligned(*m_Label);
    m_Label->FontSize = EditorStyle::FontSize;
    m_Label->VAlign = UIVAlign::Middle;

    m_TypeLabel = Add<UILabel>();
    m_TypeLabel->Anchor = m_TypeLabel->Pivot = Vec2(1.0f, 0.5f);
    m_TypeLabel->Position = Vec2(-6.0f, 0.0f);
    m_TypeLabel->Size = { TypeColumnWidth, RowHeight };
    m_TypeLabel->FontSize = EditorStyle::FontSize - 1.0f;
    m_TypeLabel->Color = EditorStyle::TextDim;
    m_TypeLabel->HAlign = UIHAlign::Right;
    m_TypeLabel->VAlign = UIVAlign::Middle;

    m_RenameField = Add<UITextArea>();
    leftAligned(*m_RenameField);
    m_RenameField->SingleLine = true;
    m_RenameField->FontSize = EditorStyle::FontSize;
    m_RenameField->TextColor = EditorStyle::TextBright;
    m_RenameField->CaretColor = EditorStyle::TextBright;
    m_RenameField->BackgroundColor = EditorStyle::PanelDark;
    m_RenameField->FocusedBorderColor = EditorStyle::Accent;
    m_RenameField->CornerRadius = 2.0f;
    m_RenameField->Padding = UIPadding(3.0f, 0.0f);
    m_RenameField->SetEnabled(false);
    m_RenameField->Submitted = [this](const String& InName) {
        if (Owner) {
            Owner->CommitRename(InName);
        }
    };
    m_RenameField->Cancelled = [this] {
        if (Owner) {
            Owner->CancelRename();
        }
    };
    m_RenameField->FocusLost = [this] {
        // Commit-on-blur; a no-op when Submitted/Cancelled already ended this rename.
        if (Owner && m_Node.Get() && Owner->GetRenamingNode() == m_Node.Get()) {
            Owner->CommitRename(m_RenameField->Text);
        }
    };

    Bind = [this] { Refresh(); };
}

void OutlinerRow::Refresh() {
    const OutlinerTab::VisibleRow* row = Owner ? Owner->GetVisibleRow(RowIndex) : nullptr;
    if (!row || !row->NodePtr) {
        m_Node = nullptr;
        return;
    }
    Node* node = row->NodePtr;
    m_Node = node;

    // The eye only appears on hover while the node is fully enabled; a crossed eye marks
    // an explicit disable, an always-visible dim open eye a node only disabled via an ancestor.
    const bool selfEnabled = node->IsSelfEnabled();
    const bool nodeEnabled = node->IsEnabled();
    m_Eye->SetEnabled(IsHovered() || !nodeEnabled);
    m_Eye->Image = Owner->GetEyeIcon(selfEnabled);
    m_Eye->Tint = selfEnabled ? (nodeEnabled ? EditorStyle::TextDim : s_DisabledText) : EditorStyle::Text;

    const float indent = EyeColumnWidth + row->Depth * IndentStep;

    m_Arrow->SetEnabled(row->HasChildren);
    m_Arrow->Position = Vec2(indent, 0.0f);
    m_Arrow->Image = Owner->GetArrowIcon(Owner->HasFilter() || Owner->IsExpanded(node));

    m_Icon->Position = Vec2(indent + 16.0f, 0.0f);
    m_Icon->Image = EditorIcons::GetNodeIcon(node->GetClass());
    m_Icon->Tint = nodeEnabled ? EditorStyle::Text : s_DisabledText;

    const float textLeft = indent + 36.0f;
    m_Label->Position = Vec2(textLeft, 0.0f);
    m_Label->Size = { 1.0_rel - UIValue::Px(textLeft + TypeColumnWidth + 12.0f), RowHeight };
    m_Label->Text = node->GetName();
    if (Owner->HasFilter()) {
        // Hits pop in the accent color even when disabled; ancestors just trace the path.
        m_Label->Color = row->Matches ? EditorStyle::AccentBright
                                      : (nodeEnabled ? EditorStyle::TextDim : s_DisabledText);
    } else if (!nodeEnabled) {
        m_Label->Color = s_DisabledText;
    } else {
        m_Label->Color = Owner->IsSelected(node) ? EditorStyle::TextBright : EditorStyle::Text;
    }

    m_TypeLabel->Text = node->GetClass().Name;
    m_TypeLabel->Color = nodeEnabled ? EditorStyle::TextDim : s_DisabledText;

    const bool renaming = Owner->GetRenamingNode() == node;
    m_Label->SetEnabled(!renaming);
    m_RenameField->SetEnabled(renaming);
    if (renaming) {
        // Offset by the field's left padding so its text stays aligned with the label's.
        m_RenameField->Position = Vec2(textLeft - 3.0f, 0.0f);
        m_RenameField->Size = { 1.0_rel - UIValue::Px(textLeft + TypeColumnWidth + 9.0f), RowHeight - 4.0f };
        if (m_RenameTarget.Get() != node) {
            m_RenameTarget = node;
            m_RenameField->Text = node->GetName();
            m_RenameField->SelectAll();
            m_RenameField->RequestFocus();
        }
    } else {
        m_RenameTarget = nullptr;
    }
}

OutlinerTab::DropMode OutlinerRow::HitZone(const Vec2& InScreenCursor) const {
    const float band = m_Geometry.Size.y * 0.30f;
    const UIRectF top(m_Geometry.Position, Vec2(m_Geometry.Size.x, band));
    const UIRectF bottom(m_Geometry.Position + Vec2(0.0f, m_Geometry.Size.y - band), Vec2(m_Geometry.Size.x, band));
    if (HitTestRect(top, InScreenCursor)) {
        return OutlinerTab::DropMode::Before;
    }
    if (HitTestRect(bottom, InScreenCursor)) {
        return OutlinerTab::DropMode::After;
    }
    return OutlinerTab::DropMode::Onto;
}

void OutlinerRow::Paint(UIDrawList& OutDrawList) {
    Node* node = m_Node.Get();
    if (!node || !Owner) {
        return;
    }
    const bool isDropRef = Owner->GetDropRef() == node;
    const OutlinerTab::DropMode dropMode = Owner->GetDropMode();

    Vec4 background(0.0f);
    if (isDropRef && dropMode == OutlinerTab::DropMode::Onto) {
        background = s_DropColor;
    } else if (Owner->IsSelected(node)) {
        background = s_SelectedColor;
    } else if (IsHovered()) {
        background = EditorStyle::TabHover;
    } else if ((RowIndex & 1) == 1) {
        background = s_AltRowColor;
    }
    if (background.a > 0.0f) {
        OutDrawList.AddRect(m_Geometry, background, m_WorldMatrix);
    }

    if (isDropRef && (dropMode == OutlinerTab::DropMode::Before || dropMode == OutlinerTab::DropMode::After)) {
        const OutlinerTab::VisibleRow* row = Owner->GetVisibleRow(RowIndex);
        const float indent = EyeColumnWidth + (row ? row->Depth : 0) * IndentStep + 16.0f;
        const float y = (dropMode == OutlinerTab::DropMode::Before) ? m_Geometry.Min().y : m_Geometry.Max().y;
        const UIRectF line(Vec2(m_Geometry.Min().x + indent, y - 1.0f),
                           Vec2(m_Geometry.Size.x - indent - 4.0f, 2.0f));
        OutDrawList.AddRect(line, EditorStyle::AccentBright, m_WorldMatrix);
    }
}

void OutlinerRow::OnPressed(const Vec2& InCursorPos) {
    m_PressPos = InCursorPos;
    m_Dragging = false;

    Node* node = m_Node.Get();
    if (!node || !Owner) {
        return;
    }

    if (m_Eye->IsEnabled() && m_Eye->HitTest(InCursorPos)) {
        node->SetEnabled(!node->IsSelfEnabled());
        return;
    }

    const OutlinerTab::VisibleRow* row = Owner->GetVisibleRow(RowIndex);
    if (!Owner->HasFilter() && row && row->HasChildren && m_Arrow->IsEnabled() && m_Arrow->HitTest(InCursorPos)) {
        Owner->ToggleExpanded(node);
        return;
    }

    KeyboardDevice* keyboard = KeyboardDevice::Instance();
    const bool toggle = keyboard && (keyboard->IsPressed(KeyCode::LeftControl) || keyboard->IsPressed(KeyCode::RightControl)
                                  || keyboard->IsPressed(KeyCode::LeftSuper) || keyboard->IsPressed(KeyCode::RightSuper));
    const bool range = keyboard && (keyboard->IsPressed(KeyCode::LeftShift) || keyboard->IsPressed(KeyCode::RightShift));

    const bool wasSoleSelected = Owner->IsSoleSelected(node);
    Owner->HandleRowClick(node, toggle, range);
    if (!toggle && !range && wasSoleSelected && m_DoubleClickTimer > 0.0f && Owner->GetRenamingNode() != node) {
        Owner->BeginRename(node);
    }
    m_DoubleClickTimer = 0.35f;
}

void OutlinerRow::OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) {
    (void)InDelta;
    Node* node = m_Node.Get();
    if (!node || !Owner || Owner->GetRenamingNode() == node) {
        return;
    }
    if (!m_Dragging) {
        const Vec2 moved = InCursorPos - m_PressPos;
        if (moved.x * moved.x + moved.y * moved.y < 16.0f) {
            return;
        }
        m_Dragging = true;
        Owner->BeginDrag(node);
    }
    Owner->DragOver(InCursorPos);
}

void OutlinerRow::OnReleased(bool InInside) {
    (void)InInside;
    if (m_Dragging && Owner) {
        Owner->EndDrag();
    }
    m_Dragging = false;
}

void OutlinerRow::OnUIUpdate(const UIFrameContext& InContext) {
    if (m_DoubleClickTimer > 0.0f) {
        m_DoubleClickTimer -= InContext.DeltaTime;
    }
}

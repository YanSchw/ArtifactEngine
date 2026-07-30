#pragma once
#include "GameFramework/UINode.h"
#include "Object/Pointer.h"
#include "UIMenuModel.h"
#include "UIContextMenu.gen.h"

class UILabel;
class UISvg;
class UIStack;
class UIScrollArea;
class UITextArea;
class UIMenuPanel;
class UIContextMenu;

class UIMenuRow : public UINode {
public:
    ARTIFACT_CLASS();

    static constexpr float ActionHeight = 24.0f;
    static constexpr float SectionHeight = 22.0f;
    static constexpr float SeparatorHeight = 7.0f;

    UIMenuRow();

    UIMenuPanel* Owner = nullptr;
    int32_t Index = 0;

    static float HeightFor(const UIMenuEntry& InEntry);

    void SetEntry(const UIMenuEntry& InEntry, float InLabelLeft, bool InArrowColumn);
    const UIMenuEntry& GetEntry() const { return m_Entry; }
    bool IsHighlighted() const;

    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnClick() override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;

private:
    void ApplyColors(bool InHighlighted);

    UIMenuEntry m_Entry;
    UISvg* m_Icon = nullptr;
    UILabel* m_Label = nullptr;
    UILabel* m_Shortcut = nullptr;
    UISvg* m_Arrow = nullptr;
    float m_HoverSeconds = 0.0f;
};

/** One popup surface: the rows of a UIMenuModel plus an optional filter field. A submenu is a
 *  panel of its own, chained to the panel and row that opened it. */
class UIMenuPanel : public UINode {
public:
    ARTIFACT_CLASS();

    UIMenuPanel();

    void Build(UIContextMenu& InMenu, UIMenuPanel* InParentPanel, const UIMenuModel& InModel);

    /** Top-left placement in canvas pixels, flipped or slid to keep the panel on the canvas. */
    void PlaceAt(const Vec2& InTopLeft);
    /** Beside a row of the parent panel. */
    void PlaceBeside(const UIRectF& InRowRect);
    /** Under a widget, aligned to its left edge. */
    void PlaceUnder(const UIRectF& InAnchorRect);

    UIContextMenu* GetMenu() const { return m_Menu.Get(); }
    UIMenuPanel* GetParentPanel() const { return m_ParentPanel.Get(); }
    UIMenuPanel* GetSubmenu() const { return m_Submenu.Get(); }
    int32_t GetHighlight() const { return m_Highlight; }

    void ActivateRow(int32_t InIndex);
    void OpenSubmenu(int32_t InIndex);
    void CloseSubmenu();

    void MoveHighlight(int32_t InDelta);
    void ActivateHighlight();
    void OpenHighlightedSubmenu();

    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnBind() override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;

private:
    void RebuildRows();
    Array<UIMenuRow*> GetRows() const;
    UIMenuRow* FindRow(int32_t InIndex) const;
    UIRectF GetCanvasRect() const;

    WeakObjectPtr<UIContextMenu> m_Menu;
    WeakObjectPtr<UIMenuPanel> m_ParentPanel;
    WeakObjectPtr<UIMenuPanel> m_Submenu;
    UIMenuModel m_Model;
    UIStack* m_List = nullptr;
    UIScrollArea* m_Scroll = nullptr;
    UITextArea* m_Search = nullptr;

    Vec2 m_DesiredTopLeft = Vec2(0.0f);
    int32_t m_Highlight = -1;
    int32_t m_HoverRow = -1;
    int32_t m_OpenRow = -1;
    float m_HoverSeconds = 0.0f;
    String m_Filter;
    bool m_RowsDirty = false;
};

/** A menu overlay: covers its canvas so a click outside dismisses the whole chain, and hosts the
 *  open panels above all other UI. Menus are opened through the static helpers, which reuse the
 *  canvas the owning node lives in. */
class UIContextMenu : public UINode {
public:
    ARTIFACT_CLASS();

    UIContextMenu();

    static UIContextMenu* OpenAt(UINode& InOwner, const Vec2& InScreenPos, const UIMenuModel& InModel);
    static UIContextMenu* OpenUnder(UINode& InAnchor, const UIMenuModel& InModel);
    static void CloseAll(UINode& InOwner);
    static bool IsOpen(UINode& InOwner);

    void Close();
    bool IsClosing() const { return m_Closing; }

    UIMenuPanel* GetRootPanel() const;
    UIMenuPanel* GetDeepestPanel() const;

    /** Shown next to InRowRect once the row under the cursor has been hovered long enough. */
    void RequestTooltip(const String& InText, const UIRectF& InRowRect);

    virtual void OnPressed(const Vec2& InCursorPos) override;
    virtual bool OnSecondaryClick(const Vec2& InCursorPos) override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;
    virtual void PaintOverlay(UIDrawList& OutDrawList) override;

private:
    static UIContextMenu* Spawn(UINode& InOwner, const UIMenuModel& InModel, UIMenuPanel*& OutPanel);
    void HandleKeys();

    bool m_Closing = false;
    String m_TooltipText;
    UIRectF m_TooltipAnchor;
};

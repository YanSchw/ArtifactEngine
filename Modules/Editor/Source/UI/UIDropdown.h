#pragma once
#include "GameFramework/UINode.h"
#include "Object/Pointer.h"
#include <functional>
#include "UIDropdown.gen.h"

class UILabel;
class UISvg;
class UITextArea;
class UIDropdownPopup;
class VectorImage;
class Texture;

struct UIDropdownOption {
    UIDropdownOption() = default;
    UIDropdownOption(const String& InLabel) : Label(InLabel) {}
    UIDropdownOption(const char* InLabel) : Label(InLabel) {}

    String Label;
    /** Drawn while Thumbnail has no view yet, or when there is none. */
    VectorImage* Icon = nullptr;
    Vec4 IconTint = Vec4(1.0f);
    Texture* Thumbnail = nullptr;
};

class UIDropdown : public UINode {
public:
    ARTIFACT_CLASS();

    UIDropdown();
    virtual ~UIDropdown();

    /** Non-empty puts a filter field above the options, matching labels as you type. */
    String SearchPlaceholder;

    std::function<String()> GetSelectedLabel;
    std::function<Array<UIDropdownOption>()> GetOptions;
    std::function<int32_t()> GetSelectedIndex;
    std::function<void(int32_t)> SelectionChanged;

    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnBind() override;
    virtual void OnClick() override;

    bool IsOpen() const;
    void ClosePopup();

private:
    UILabel* m_ValueLabel = nullptr;
    UISvg* m_Arrow = nullptr;
    WeakObjectPtr<UIDropdownPopup> m_Popup;
};

class UIDropdownPopup : public UINode {
public:
    ARTIFACT_CLASS();

    UIDropdownPopup();

    void Build(UIDropdown* InOwner, const Array<UIDropdownOption>& InOptions, int32_t InSelected);
    /** Hides and flags the popup; the owning dropdown deletes it on its next bind */
    void RequestClose() { m_CloseRequested = true; SetEnabled(false); }
    bool IsCloseRequested() const { return m_CloseRequested; }

    virtual void OnBind() override;
    virtual void OnPressed(const Vec2& InCursorPos) override;

private:
    void RebuildRows();
    void AddRow(int32_t InIndex);
    float RowHeight() const;

    WeakObjectPtr<UIDropdown> m_Owner;
    UINode* m_Panel = nullptr;
    UITextArea* m_Search = nullptr;
    UINode* m_List = nullptr;
    Array<UIDropdownOption> m_Options;
    String m_Filter;
    float m_ListHeight = 0.0f;
    int32_t m_Selected = -1;
    bool m_HasThumbnails = false;
    bool m_CloseRequested = false;
};

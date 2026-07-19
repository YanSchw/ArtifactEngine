#pragma once
#include "GameFramework/UINode.h"
#include "Object/Pointer.h"
#include <functional>
#include "UIDropdown.gen.h"

class UILabel;
class UISvg;
class UIDropdownPopup;

class UIDropdown : public UINode {
public:
    ARTIFACT_CLASS();

    UIDropdown();
    virtual ~UIDropdown();

    std::function<String()> GetSelectedLabel;
    std::function<Array<String>()> GetOptions;
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

    void Build(UIDropdown* InOwner, const Array<String>& InOptions, int32_t InSelected);
    /** Hides and flags the popup; the owning dropdown deletes it on its next bind */
    void RequestClose() { m_CloseRequested = true; SetEnabled(false); }
    bool IsCloseRequested() const { return m_CloseRequested; }

    virtual void OnBind() override;
    virtual void OnPressed(const Vec2& InCursorPos) override;

private:
    WeakObjectPtr<UIDropdown> m_Owner;
    UINode* m_Panel = nullptr;
    bool m_CloseRequested = false;
};

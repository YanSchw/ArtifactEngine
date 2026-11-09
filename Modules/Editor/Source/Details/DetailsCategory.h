#pragma once
#include "DetailsItem.h"
#include <functional>
#include "DetailsCategory.gen.h"

class UIButton;
class UILabel;
class UISvg;
class UIVStack;

/** A collapsible section of the Details panel. Nests: its body holds rows and sub-categories. */
class DetailsCategory : public DetailsItem {
public:
    ARTIFACT_CLASS();

    static constexpr float HeaderHeight = 24.0f;
    static constexpr float BodyPadding = 4.0f;

    DetailsCategory();

    void SetTitle(const String& InTitle);
    UINode* GetBody() const;
    /** Hosts an editor in the header between the arrow and the title. */
    UINode* AddHeaderEditor(float InWidth);
    UINode* AddHeaderValue();
    String GetStateKey() const;

    /** While this returns false the rows inside are drawn dimmed. */
    std::function<bool()> IsActive;
    std::function<void()> ResetAction;
    std::function<bool()> IsOverridden;

    virtual float MeasureHeight(const String& InFilter) const override;
    virtual void OnBind() override;
    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnPressed(const Vec2& InCursorPos) override;

private:
    bool TitleMatches(const String& InFilter) const;

    String m_Title;
    UILabel* m_TitleLabel = nullptr;
    UISvg* m_Arrow = nullptr;
    UIVStack* m_Body = nullptr;
    UINode* m_HeaderEditor = nullptr;
    UINode* m_HeaderValue = nullptr;
    UIButton* m_ResetButton = nullptr;
    float m_HeaderEditorWidth = 0.0f;
    bool m_Expanded = true;
    bool m_StateRestored = false;
};

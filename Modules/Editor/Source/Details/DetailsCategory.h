#pragma once
#include "DetailsItem.h"
#include "DetailsCategory.gen.h"

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
    bool m_Expanded = true;
};

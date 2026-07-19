#pragma once
#include "DetailsItem.h"
#include <functional>
#include "DetailsRow.gen.h"

class UILabel;
class UIButton;

/** One label|value line. */
class DetailsRow : public DetailsItem {
public:
    ARTIFACT_CLASS();

    static constexpr float RowHeight = 24.0f;
    static constexpr float ResetColumnWidth = 24.0f;

    DetailsRow();

    void SetLabel(const String& InLabel);
    const String& GetLabel() const { return m_Label; }
    UINode* GetValueHost() const { return m_ValueHost; }

    std::function<void()> ResetAction;

    virtual float MeasureHeight(const String& InFilter) const override;
    virtual void OnBind() override;
    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnPressed(const Vec2& InCursorPos) override;
    virtual void OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) override;
    virtual void OnReleased(bool InInside) override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;

private:
    bool IsNearSplitter(const Vec2& InScreenPos) const;

    String m_Label;
    UILabel* m_NameLabel = nullptr;
    UIButton* m_ResetButton = nullptr;
    UINode* m_ValueHost = nullptr;
    bool m_DraggingSplit = false;
};

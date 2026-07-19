#pragma once
#include "GameFramework/UINode.h"
#include <functional>
#include <limits>
#include "UIDragNumber.gen.h"

class UILabel;
class UITextArea;

class UIDragNumber : public UINode {
public:
    ARTIFACT_CLASS();

    UIDragNumber();

    std::function<double()> Get;
    std::function<void(double)> Set;
    bool Integer = false;
    int32_t Decimals = 2;
    double Sensitivity = 0.05;
    double MinValue = std::numeric_limits<double>::lowest();
    double MaxValue = std::numeric_limits<double>::max();
    String Prefix;
    Vec4 PrefixColor = Vec4(0.0f);
    std::function<void()> PrefixClicked;

    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnBind() override;
    virtual void OnPressed(const Vec2& InCursorPos) override;
    virtual void OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) override;
    virtual void OnReleased(bool InInside) override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;

private:
    void BeginEdit();
    void CommitEdit(const String& InText);
    void EndEdit();
    void WriteValue(double InValue);
    String FormatValue(double InValue) const;
    float PrefixWidth() const { return PrefixColor.a > 0.0f ? 16.0f : 0.0f; }
    bool IsOverPrefix(const Vec2& InScreenPos) const;

    UILabel* m_PrefixLabel = nullptr;
    UILabel* m_ValueLabel = nullptr;
    UITextArea* m_EditField = nullptr;
    bool m_Editing = false;
    bool m_Dragging = false;
    bool m_PressedOnPrefix = false;
    double m_DragValue = 0.0;
    Vec2 m_PressPos = Vec2(0.0f);
};

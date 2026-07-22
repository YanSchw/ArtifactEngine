#pragma once
#include "GameFramework/UIButton.h"
#include "UIMajorTabHandle.gen.h"

class MajorTab;
class EditorWindow;
class UISvg;
class UILabel;
class VectorImage;

/** A MajorTab's entry in the bottom tab bar. */
class UIMajorTabHandle : public UIButton {
public:
    ARTIFACT_CLASS();

    UIMajorTabHandle();

    MajorTab* Tab = nullptr;
    EditorWindow* OwnerWindow = nullptr;

    void SetContent(const String& InCaption, VectorImage* InIcon);
    void SetActive(bool InActive) { m_Active = InActive; }

    virtual void OnUIUpdate(const UIFrameContext& InContext) override;
    virtual void OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) override;
    virtual void OnPressed(const Vec2& InCursorPos) override;
    virtual void OnReleased(bool InInside) override;

private:
    float ExpandedWidth() const;

    UISvg* m_Icon = nullptr;
    UILabel* m_Label = nullptr;
    float m_TextWidth = 0.0f;
    float m_Expand = 0.0f;
    bool m_Active = false;
    Vec2 m_PressPosition = Vec2(0.0f);
};

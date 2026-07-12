#pragma once
#include "GameFramework/UIButton.h"
#include "UIMajorTabHandle.gen.h"

class MajorTab;
class EditorWindow;

/** A MajorTab's entry in the bottom tab bar: click activates, drag moves the tab to another
 *  EditorWindow or tears it off into a new one. */
class UIMajorTabHandle : public UIButton {
public:
    ARTIFACT_CLASS();

    MajorTab* Tab = nullptr;
    EditorWindow* OwnerWindow = nullptr;

    virtual void OnDrag(const Vec2& InCursorPos, const Vec2& InDelta) override;
    virtual void OnPressed(const Vec2& InCursorPos) override;
    virtual void OnReleased(bool InInside) override;

private:
    Vec2 m_PressPosition = Vec2(0.0f);
};

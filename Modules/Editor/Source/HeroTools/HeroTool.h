#pragma once
#include "Object/Object.h"
#include "Common/String.h"
#include "HeroTool.gen.h"

class UINode;
class VectorImage;
class EditorWindow;

class HeroTool : public Object {
public:
    ARTIFACT_CLASS();

    virtual String GetTitle() const { return "Tool"; }
    /** Status-bar button side: false docks it on the left (before the tabs), true on the right. */
    virtual bool IsRightAligned() const { return false; }
    virtual float GetStatusButtonWidth() const { return 156.0f; }

    virtual void BuildDrawer(UINode& InBody) { (void)InBody; }
    virtual void Tick(float InDeltaTime) { (void)InDeltaTime; }
    virtual bool BuildStatusWidget(UINode& InButton) { (void)InButton; return false; }

    void SetOwnerWindow(EditorWindow* InWindow) { m_OwnerWindow = InWindow; }
    EditorWindow* GetOwnerWindow() const { return m_OwnerWindow; }

private:
    EditorWindow* m_OwnerWindow = nullptr;
};

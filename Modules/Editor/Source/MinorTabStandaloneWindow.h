#pragma once
#include "ThemedWindow.h"
#include "MinorTabStandaloneWindow.gen.h"

class MinorTab;
class MajorTab;

/** A free-floating window hosting a single MinorTab that was dragged out of a dock area.
 *  It lives and hides with the MajorTab that owns the tab; closing it docks the tab back. */
class MinorTabStandaloneWindow : public ThemedWindow {
public:
    ARTIFACT_CLASS();
protected:
    MinorTabStandaloneWindow(const WindowParams& InParams);
public:
    static SharedObjectPtr<MinorTabStandaloneWindow> Create(MinorTab* InTab, MajorTab* InOwner, const Vec2& InScreenPos);

    MinorTab* GetTab() const { return m_Tab; }

    virtual bool OnCloseRequested() override;

private:
    MinorTab* m_Tab = nullptr;
    MajorTab* m_Owner = nullptr;
};

#pragma once
#include "GameFramework/WorldSubsystem.h"
#include "InputSystem/CursorIcon.h"
#include "Object/Pointer.h"
#include "UIInput.h"
#include "UIWorld.gen.h"

class UICanvas;
class UIDrawList;
class UINode;

/** Routes input to the UI of its World. Only UI routed this way is drawn by the UIRenderPass. */
class UIWorld : public WorldSubsystem {
public:
    ARTIFACT_CLASS();

    static UIWorld* Get(World* InWorld);

    virtual void Shutdown() override;

    /** Updates the World's UI for this frame; the topmost canvas under the cursor takes the pointer. */
    void Route(const Vec2& InViewportSize, const UIFrameContext& InContext);

    /** The canvases routed since the last call, in paint order. */
    Array<UICanvas*> ConsumeRoutedCanvases();
    /** Paints a routed canvas, returning the projection to render it with. */
    Mat4 Paint(UICanvas& InCanvas, UIDrawList& OutDrawList) const;

    CursorIcon GetDesiredCursor() const { return m_DesiredCursor; }

private:
    void CollectRoots(Array<UICanvas*>& OutCanvases, Array<UINode*>& OutLooseRoots) const;

    SharedObjectPtr<UICanvas> m_LooseRootCanvas;
    Array<WeakObjectPtr<UINode>> m_LooseRoots;
    Array<WeakObjectPtr<UICanvas>> m_Routed;
    CursorIcon m_DesiredCursor = CursorIcon::Arrow;
};
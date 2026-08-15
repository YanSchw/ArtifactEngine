#pragma once
#include "GameFramework/UINode.h"
#include "Object/Pointer.h"
#include "Common/String.h"
#include "UITooltip.gen.h"

/** One per canvas, created on demand. */
class UITooltipLayer : public UINode {
public:
    ARTIFACT_CLASS();

    UITooltipLayer();

    static UITooltipLayer* EnsureFor(UINode& InOwner);

    void Show(const String& InText, const UIRectF& InAnchor);

    virtual void PaintOverlay(UIDrawList& OutDrawList) override;

private:
    String m_Text;
    UIRectF m_Anchor;
};

/** Attach one to any node to give it a hover tooltip. */
class UITooltip : public UINode {
public:
    ARTIFACT_CLASS();

    UITooltip();

    String Text;

    virtual void OnUIUpdate(const UIFrameContext& InContext) override;

private:
    WeakObjectPtr<UITooltipLayer> m_Layer;
    float m_HoverSeconds = 0.0f;
};

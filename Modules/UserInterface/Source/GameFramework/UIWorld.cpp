#include "UIWorld.h"
#include "UICanvas.h"
#include "GameFramework/World.h"

UIWorld* UIWorld::Get(World* InWorld) {
    return InWorld ? InWorld->GetSubsystem<UIWorld>() : nullptr;
}

void UIWorld::Shutdown() {
    m_LooseRootCanvas = nullptr;
}

void UIWorld::CollectRoots(Array<UICanvas*>& OutCanvases, Array<UINode*>& OutLooseRoots) const {
    for (Node* node : GetWorld().GetAllNodes()) {
        UINode* uiNode = node->As<UINode>();
        if (!uiNode || !uiNode->IsEnabled() || (node->GetParent() && node->GetParent()->As<UINode>())) {
            continue;
        }
        if (UICanvas* canvas = uiNode->As<UICanvas>()) {
            OutCanvases.Add(canvas);
        } else {
            OutLooseRoots.Add(uiNode);
        }
    }
}

void UIWorld::Route(const Vec2& InViewportSize, const UIFrameContext& InContext) {
    Array<UICanvas*> canvases;
    Array<UINode*> looseRoots;
    CollectRoots(canvases, looseRoots);
    m_Routed.Clear();
    m_LooseRoots.Clear();
    m_DesiredCursor = CursorIcon::Arrow;

    UIFrameContext context = InContext;
    bool pointerTaken = false;
    for (int32_t i = canvases.Size() - 1; i >= 0; i--) {
        UICanvas* canvas = canvases[i];
        canvas->UpdateFrame(InViewportSize, context);
        if (!pointerTaken && canvas->GetHoveredNode()) {
            pointerTaken = true;
            m_DesiredCursor = canvas->GetDesiredCursor();
            context.CursorPosition = Vec2(-1.0e6f);
            context.CursorPressedThisFrame = false;
            context.SecondaryPressedThisFrame = false;
            context.ScrollDelta = Vec2(0.0f);
        }
    }

    if (!looseRoots.IsEmpty()) {
        if (!m_LooseRootCanvas) {
            m_LooseRootCanvas = Object::Create<UICanvas>();
        }
        m_LooseRootCanvas->UpdateFrame(InViewportSize, context);
        for (UINode* root : looseRoots) {
            root->UpdateSubtreeFrame(m_LooseRootCanvas->GetGeometry(), context);
            m_LooseRoots.Add(root);
        }
        m_Routed.Add(m_LooseRootCanvas.Get());
    }
    for (UICanvas* canvas : canvases) {
        m_Routed.Add(canvas);
    }
}

Array<UICanvas*> UIWorld::ConsumeRoutedCanvases() {
    Array<UICanvas*> canvases;
    for (const WeakObjectPtr<UICanvas>& canvas : m_Routed) {
        if (canvas.Get()) {
            canvases.Add(canvas.Get());
        }
    }
    m_Routed.Clear();
    return canvases;
}

Mat4 UIWorld::Paint(UICanvas& InCanvas, UIDrawList& OutDrawList) const {
    const Mat4 projection = InCanvas.PaintFrame(OutDrawList);
    if (&InCanvas == m_LooseRootCanvas.Get()) {
        for (const WeakObjectPtr<UINode>& root : m_LooseRoots) {
            if (UINode* node = root.Get()) {
                node->PaintSubtreeFrame(OutDrawList);
            }
        }
    }
    return projection;
}
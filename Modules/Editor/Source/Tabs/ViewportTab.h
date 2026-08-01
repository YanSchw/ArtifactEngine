#pragma once
#include "MinorTab.h"
#include "Object/Pointer.h"
#include "ViewportTab.gen.h"

class EditorCamera;
class RenderPipeline;
class RenderTargetTexture;
class UIViewportSurface;
class GizmoLayer;
class GizmoRenderer;
class TransformGizmo;
class VectorImage;

class ViewportTab : public MinorTab {
public:
    ARTIFACT_CLASS();

    ViewportTab();

    virtual String GetTabTitle() const override { return "Viewport"; }
    virtual VectorImage* GetTabIcon() const override;

    virtual void OnUIUpdate(const UIFrameContext& InContext) override;
    virtual bool OnScroll(const Vec2& InDelta) override;

    EditorCamera* GetCamera() const { return m_Camera.Get(); }

private:
    void BuildToolBar(UINode& InToolBar);
    void UpdateToolShortcuts();
    void PickAt(const Vec2& InRenderPixel);

    SharedObjectPtr<EditorCamera> m_Camera;
    SharedObjectPtr<RenderPipeline> m_Pipeline;
    SharedObjectPtr<RenderTargetTexture> m_SceneTexture;
    SharedObjectPtr<GizmoLayer> m_GizmoLayer;
    SharedObjectPtr<GizmoRenderer> m_GizmoRenderer;
    SharedObjectPtr<TransformGizmo> m_TransformGizmo;
    UIViewportSurface* m_ViewportArea = nullptr;
};

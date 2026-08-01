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
class UILayoutGizmo;
class UICanvas;
class UIRenderer;
class FrameBuffer;
class VectorImage;

class ViewportTab : public MinorTab {
public:
    ARTIFACT_CLASS();

    ViewportTab();
    virtual ~ViewportTab();

    virtual String GetTabTitle() const override { return "Viewport"; }
    virtual VectorImage* GetTabIcon() const override;

    virtual void OnUIUpdate(const UIFrameContext& InContext) override;
    virtual bool OnScroll(const Vec2& InDelta) override;

    EditorCamera* GetCamera() const { return m_Camera.Get(); }

    void SetDesignMode(bool InDesignMode) { m_DesignMode = InDesignMode; }
    bool IsDesignMode() const { return m_DesignMode; }

private:
    void BuildToolBar(UINode& InToolBar);
    void BuildSceneTools(UINode& InParent);
    void BuildDesignTools(UINode& InParent);
    void UpdateToolShortcuts();

    void RenderScene(const UIFrameContext& InContext, const UIRectF& InRect);
    void RenderDesign(const UIFrameContext& InContext, const UIRectF& InRect);
    UICanvas* FindWorldCanvas() const;
    void CollectDesignRoots(Array<UINode*>& OutRoots) const;
    void EnsureDesignTarget(uint32_t InWidth, uint32_t InHeight);
    Vec2 DesignCanvasSize() const;
    Vec2 CanvasFromViewport(const Vec2& InViewportPixel) const;
    float CanvasPixelScale() const;

    void OnViewportPressed(const Vec2& InRenderPixel);
    void OnViewportDragged(const Vec2& InRenderPixel);
    void OnViewportReleased();
    void PickAt(const Vec2& InRenderPixel);
    void PickUIAt(const Vec2& InCanvasPoint);

    SharedObjectPtr<EditorCamera> m_Camera;
    SharedObjectPtr<RenderPipeline> m_Pipeline;
    SharedObjectPtr<RenderTargetTexture> m_SceneTexture;
    SharedObjectPtr<GizmoLayer> m_GizmoLayer;
    SharedObjectPtr<GizmoRenderer> m_GizmoRenderer;
    SharedObjectPtr<TransformGizmo> m_TransformGizmo;
    UIViewportSurface* m_ViewportArea = nullptr;

    SharedObjectPtr<UILayoutGizmo> m_LayoutGizmo;
    SharedObjectPtr<UICanvas> m_DesignCanvas;
    SharedObjectPtr<FrameBuffer> m_DesignTarget;
    UIRenderer* m_DesignRenderer = nullptr;
    Mat4 m_DesignProjection = Mat4(1.0f);
    Vec2 m_DesignViewport = Vec2(1.0f);
    bool m_DesignMode = false;
    bool m_PreviewInput = false;
};

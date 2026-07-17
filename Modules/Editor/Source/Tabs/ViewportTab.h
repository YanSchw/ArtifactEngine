#pragma once
#include "MinorTab.h"
#include "Object/Pointer.h"
#include "ViewportTab.gen.h"

class EditorCamera;
class RenderPipeline;
class RenderTargetTexture;
class UIImage;

class ViewportTab : public MinorTab {
public:
    ARTIFACT_CLASS();

    ViewportTab();

    virtual String GetTabTitle() const override { return "Viewport"; }

    virtual void OnUIUpdate(const UIFrameContext& InContext) override;
    virtual bool OnScroll(const Vec2& InDelta) override;

    EditorCamera* GetCamera() const { return m_Camera.Get(); }

private:
    void BuildToolBar(UINode& InToolBar);

    SharedObjectPtr<EditorCamera> m_Camera;
    SharedObjectPtr<RenderPipeline> m_Pipeline;
    SharedObjectPtr<RenderTargetTexture> m_SceneTexture;
    UIImage* m_ViewportArea = nullptr;
    int m_ActiveTool = 0;
};

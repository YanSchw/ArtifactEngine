#pragma once
#include "Window.h"
#include "GameFramework/UINode.h"
#include "ThemedWindow.gen.h"

class UICanvas;
class UIQuad;
class UILabel;
class UIRenderer;
class UISvg;
class VectorImage;

/** A general-purpose window with the editor's chrome. */
class ThemedWindow : public Window {
public:
    ARTIFACT_CLASS();
protected:
    ThemedWindow(const WindowParams& InParams);
public:
    virtual ~ThemedWindow();

    static SharedObjectPtr<ThemedWindow> Create(WindowParams InParams);

    /** Window coordinates per canvas unit; the sizes in WindowParams are given in canvas units. */
    static float GetUIScale();
    Vec2 CanvasToScreen(const Vec2& InCanvasPoint) const;

    UICanvas* GetCanvas() const { return m_Canvas; }
    UINode* GetContentRoot() const { return m_ContentRoot; }
    void SetTitleText(const String& InText);
    void SetTitleLogo(VectorImage* InLogo);

    virtual void RenderWindow(double InDeltaTime);
    /** Reacting to the user closing the window; return true to let the engine destroy it. */
    virtual bool OnCloseRequested() { return true; }

    virtual bool HitTestTitleBar(const Vec2& InPoint) const override;

    bool ContainsScreenPoint(const Vec2& InScreenPoint) const;
    static ThemedWindow* FindWindowAtScreenPoint(const Vec2& InScreenPoint, const ThemedWindow* InIgnored = nullptr);

    static const Array<SharedObjectPtr<ThemedWindow>>& GetAllWindows();
    static void RegisterWindow(const SharedObjectPtr<ThemedWindow>& InWindow);
    static void DestroyWindow(ThemedWindow* InWindow);
    static void DestroyAllWindows();

protected:
    virtual void PreUIRender(double InDeltaTime) { (void)InDeltaTime; }
    virtual void BuildFrameContext(struct UIFrameContext& OutContext, double InDeltaTime);

    /** Releases the GPU resources whose render target is this window. */
    virtual void ReleaseResources();

    UICanvas* m_Canvas = nullptr;
    UIQuad* m_TitleBar = nullptr;
    UINode* m_ContentRoot = nullptr;
    Array<UINode*> m_TitleBarButtons;
    UIRenderer* m_UIRenderer = nullptr;
    double m_FrameSeconds = 0.0;

private:
    void BuildChrome(const String& InTitle);

    UILabel* m_TitleLabel = nullptr;
    UISvg* m_TitleLogo = nullptr;
    float m_TitleRightInset = 12.0f;
    bool m_WasCursorDown = false;
    bool m_WasSecondaryDown = false;
};

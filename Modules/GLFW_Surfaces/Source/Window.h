#pragma once
#include "Rendering/Surface.h"
#include "Common/Types.h"
#include "Common/Array.h"
#include "Common/String.h"
#include "InputSystem/CursorIcon.h"
#include "Window.gen.h"

struct WindowParams {
    std::string Title;
    uint32_t Width;
    uint32_t Height;
    bool Fullscreen = false;
    /* Strips the native title bar (keeping resize frame, snapping and animations) so the
     * application can draw its own chrome. */
    bool EditorStyle = false;
};

class Window : public Surface {
public:
    ARTIFACT_CLASS();
protected:
    Window(const WindowParams& InParams);
public:
    virtual ~Window();

    void TickWindow();

    virtual uint32_t GetWidth() const override;
    virtual uint32_t GetHeight() const override;
    void SetResizedFlag(bool InFlag);
    bool WasWindowResized() const;

    bool ShouldClose() const;
    void PollEvents();

    void Minimize();
    void ToggleMaximize();
    bool IsMaximized() const;
    bool IsMinimized() const;
    void Close();

    void Show();
    void Hide();
    bool IsVisible() const;
    bool IsFocused() const;
    void Focus();

    Vec2 GetPosition() const;
    void SetPosition(const Vec2& InScreenPos);

    Vec2 GetCursorPosition() const;
    bool IsMouseButtonDown(int32_t InButton) const;
    void AccumulateScroll(const Vec2& InDelta) { m_ScrollAccum += InDelta; }
    /** Scroll accumulated on this window since the last call. */
    Vec2 ConsumeScrollDelta();
    static Vec2 ConsumeGlobalScrollDelta();

    void AccumulateTextInput(uint32_t InCodepoint);
    /** Characters typed on this window since the last call, layout- and modifier-resolved. */
    String ConsumeTextInput();

    /** Whether a client-area point (window coordinates) lies in the draggable region of an
     *  application-drawn title bar. Queried by the platform for EditorStyle windows; the OS
     *  then handles dragging, snapping and double-click itself. */
    virtual bool HitTestTitleBar(const Vec2& InPoint) const { (void)InPoint; return false; }

    // Lock + hide the cursor for relative mouse look (GLFW_CURSOR_DISABLED),
    // or restore the normal visible cursor.
    void SetCursorLocked(bool InLocked);
    bool IsCursorLocked() const;

    /** Pointer shape shown while the cursor is over this window. */
    void SetCursorIcon(CursorIcon InIcon);

    static SharedObjectPtr<Window> Create(const WindowParams& InParams);
    static Window* GetInstance();
    static Window* GetFocusedWindow();
    /** The focused window's GLFW handle (primary window when none is focused). */
    static struct GLFWwindow* GetGLFWwindow();
    struct GLFWwindow* GetNativeWindow() const { return m_Window; }
private:
    WindowParams m_Params;
    struct GLFWwindow* m_Window = nullptr;
    bool m_CursorLocked = false;
    bool m_Resized = false;
    Vec2 m_ScrollAccum = Vec2(0.0f);
    String m_TextInputAccum;
    CursorIcon m_CursorIcon = CursorIcon::Arrow;
};

#pragma once
#include "Rendering/Surface.h"
#include "Common/Types.h"
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
    void Close();

    /** Whether a client-area point (window coordinates) lies in the draggable region of an
     *  application-drawn title bar. Queried by the platform for EditorStyle windows; the OS
     *  then handles dragging, snapping and double-click itself. */
    virtual bool HitTestTitleBar(const Vec2& InPoint) const { (void)InPoint; return false; }

    // Lock + hide the cursor for relative mouse look (GLFW_CURSOR_DISABLED),
    // or restore the normal visible cursor.
    void SetCursorLocked(bool InLocked);
    bool IsCursorLocked() const;

    static SharedObjectPtr<Window> Create(const WindowParams& InParams);
    static Window* GetInstance();
    static struct GLFWwindow* GetGLFWwindow();
private:
    WindowParams m_Params;
    struct GLFWwindow* m_Window = nullptr;
    bool m_CursorLocked = false;
};

#pragma once
#include "Rendering/Surface.h"
#include "Window.gen.h"

struct WindowParams {
    std::string Title;
    uint32_t Width;
    uint32_t Height;
    bool Fullscreen = false;
};

class Window : public Surface {
public:
    ARTIFACT_CLASS();
private:
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
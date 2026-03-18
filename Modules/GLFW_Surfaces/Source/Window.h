#pragma once

#include "Common/Types.h"
#include "Core/Object.h"
#include "Core/Pointer.h"
#include "Window.gen.h"

struct WindowParams {
    std::string Title;
    uint32_t Width;
    uint32_t Height;
    bool Fullscreen = false;
};

class Window : public Object {
public:
    ARTIFACT_CLASS();
private:
    Window(const WindowParams& InParams);
public:
    virtual ~Window();

    void TickWindow();

    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    void SetResizedFlag(bool InFlag);
    bool WasWindowResized() const;

    bool ShouldClose() const;
    void PollEvents();

    static SharedObjectPtr<Window> Create(const WindowParams& InParams);
    static Window* GetInstance();
    static struct GLFWwindow* GetGLFWwindow();
private:
    WindowParams m_Params;
    struct GLFWwindow* m_Window = nullptr;
};
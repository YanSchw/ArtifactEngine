#include "Window.h"
#include "Core/Log.h"

#include "GLFWKeyboardDevice.h"
#include "GLFWMouseDevice.h"
#include "GLFWGamepadDevice.h"
#include "InputSystem/InputSystem.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

static Window* s_Instance = nullptr;
static bool s_WindowResized = false;
static void OnWindowResized(GLFWwindow* InWindow, int InWidth, int InHeight) {
    s_WindowResized = true;
}

Window::Window(const WindowParams& InParams) {
    s_Instance = this;
    if (!glfwInit()) {
        AE_ERROR("Failed to initialize GLFW");
        return;
    }
    if (!glfwVulkanSupported()) {
        AE_ERROR("GLFW: Vulkan not supported");
        return;
    }

    // Set GLFW window hints for Vulkan
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_TITLEBAR, InParams.EditorStyle ? GLFW_FALSE : GLFW_TRUE);

    m_Params = InParams;
    // TODO: Window SHARED CONTEXT
    m_Window = glfwCreateWindow(m_Params.Width, m_Params.Height, m_Params.Title.c_str(), nullptr, nullptr);
    if (!m_Window) {
        AE_ERROR("Failed to create GLFW window");
        glfwTerminate();
        return;
    }
    glfwSetWindowUserPointer(m_Window, this);
    glfwSetWindowSizeCallback(m_Window, OnWindowResized);

    if (m_Params.EditorStyle) {
        glfwSetTitlebarHitTestCallback(m_Window, [](GLFWwindow* InWindow, int InX, int InY, int* OutHit) {
            Window* self = static_cast<Window*>(glfwGetWindowUserPointer(InWindow));
            *OutHit = (self && self->HitTestTitleBar(Vec2((float)InX, (float)InY))) ? 1 : 0;
        });
    }

    // Spin up the global input devices the first time a window is created.
    static bool s_InputDevicesCreated = false;
    if (!s_InputDevicesCreated) {
        s_InputDevicesCreated = true;
        InputSystem::Get().AddDevice(new GLFWKeyboardDevice());
        InputSystem::Get().AddDevice(new GLFWMouseDevice());
        InputSystem::Get().AddDevice(new GLFWGamepadDevice());
    }
}
Window::~Window() {
    // Destructor implementation
    AE_INFO("Destroying window");
}

void Window::TickWindow() {

}
uint32_t Window::GetWidth() const {
    int w, h;
    glfwGetWindowSize(m_Window, &w, &h);
    return w;
}
uint32_t Window::GetHeight() const {
    int w, h;
    glfwGetWindowSize(m_Window, &w, &h);
    return h;
}

void Window::SetResizedFlag(bool InFlag) {
    s_WindowResized = InFlag;
}

bool Window::WasWindowResized() const {
    return s_WindowResized;
}

bool Window::ShouldClose() const {
    return glfwWindowShouldClose(m_Window);
}

void Window::PollEvents() {
    glfwPollEvents();
}

void Window::Minimize() {
    glfwIconifyWindow(m_Window);
}

void Window::ToggleMaximize() {
    if (IsMaximized()) {
        glfwRestoreWindow(m_Window);
    } else {
        glfwMaximizeWindow(m_Window);
    }
}

bool Window::IsMaximized() const {
    return glfwGetWindowAttrib(m_Window, GLFW_MAXIMIZED) == GLFW_TRUE;
}

void Window::Close() {
    glfwSetWindowShouldClose(m_Window, GLFW_TRUE);
}

void Window::SetCursorLocked(bool InLocked) {
    m_CursorLocked = InLocked;
    glfwSetInputMode(m_Window, GLFW_CURSOR, InLocked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

    // Raw motion (unaccelerated) gives cleaner relative look while locked.
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(m_Window, GLFW_RAW_MOUSE_MOTION, InLocked ? GLFW_TRUE : GLFW_FALSE);
    }
}

bool Window::IsCursorLocked() const {
    return m_CursorLocked;
}

SharedObjectPtr<Window> Window::Create(const WindowParams& InParams) {
    return SharedObjectPtr<Window>(new Window(InParams));
}

Window* Window::GetInstance() {
    return s_Instance;
}

GLFWwindow* Window::GetGLFWwindow() {
    return Window::GetInstance()->m_Window;
}
#include "Window.h"
#include "Core/Log.h"
#include "Rendering/RenderingAPI.h"

#include "GLFWKeyboardDevice.h"
#include "GLFWMouseDevice.h"
#include "GLFWGamepadDevice.h"
#include "InputSystem/InputSystem.h"
#include "InputSystem/Clipboard.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

static Window* s_Instance = nullptr;
static Array<Window*> s_AllWindows;
static Vec2 s_GlobalScrollAccum = Vec2(0.0f);

static void OnWindowResized(GLFWwindow* InWindow, int InWidth, int InHeight) {
    if (Window* self = static_cast<Window*>(glfwGetWindowUserPointer(InWindow))) {
        self->SetResizedFlag(true);
    }
}

static void OnWindowScroll(GLFWwindow* InWindow, double InOffsetX, double InOffsetY);
static void OnWindowChar(GLFWwindow* InWindow, unsigned int InCodepoint);

Window::Window(const WindowParams& InParams) {
    if (!s_Instance) {
        s_Instance = this;
    }
    s_AllWindows.Add(this);

    if (!glfwInit()) {
        AE_ERROR("Failed to initialize GLFW");
        return;
    }
    if (!glfwVulkanSupported()) {
        AE_ERROR("GLFW: Vulkan not supported");
        return;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_TITLEBAR, InParams.EditorStyle ? GLFW_FALSE : GLFW_TRUE);

    m_Params = InParams;
    m_Window = glfwCreateWindow(m_Params.Width, m_Params.Height, m_Params.Title.c_str(), nullptr, nullptr);
    if (!m_Window) {
        AE_ERROR("Failed to create GLFW window");
        return;
    }
    glfwSetWindowUserPointer(m_Window, this);
    glfwSetWindowSizeCallback(m_Window, OnWindowResized);
    glfwSetScrollCallback(m_Window, OnWindowScroll);
    glfwSetCharCallback(m_Window, OnWindowChar);

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

        Clipboard::SetBackend(
            [] {
                const char* text = glfwGetClipboardString(nullptr);
                return String(text ? text : "");
            },
            [](const String& InText) { glfwSetClipboardString(nullptr, InText.c_str()); });
    }
}

Window::~Window() {
    s_AllWindows.Remove(this);
    if (s_Instance == this) {
        s_Instance = s_AllWindows.IsEmpty() ? nullptr : s_AllWindows[0];
    }
    if (m_Window) {
        // The swapchain and VkSurface must go before their GLFW window.
        if (RenderingAPI::GetInstance()) {
            RenderingAPI::GetInstance()->DestroySurfaceResources(this);
        }
        AE_INFO("Destroying GLFW window");
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }
}

static void OnWindowScroll(GLFWwindow* InWindow, double InOffsetX, double InOffsetY) {
    s_GlobalScrollAccum += Vec2((float)InOffsetX, (float)InOffsetY);
    if (Window* self = static_cast<Window*>(glfwGetWindowUserPointer(InWindow))) {
        self->AccumulateScroll(Vec2((float)InOffsetX, (float)InOffsetY));
    }
}

static void OnWindowChar(GLFWwindow* InWindow, unsigned int InCodepoint) {
    if (Window* self = static_cast<Window*>(glfwGetWindowUserPointer(InWindow))) {
        self->AccumulateTextInput((uint32_t)InCodepoint);
    }
}

void Window::AccumulateTextInput(uint32_t InCodepoint) {
    // The UI font atlas only covers printable ASCII; other codepoints are dropped here rather
    // than degrading into per-byte garbage in UTF-8-unaware text handling downstream.
    if (InCodepoint >= 32 && InCodepoint < 127) {
        m_TextInputAccum += (char)InCodepoint;
    }
}

String Window::ConsumeTextInput() {
    String text = m_TextInputAccum;
    m_TextInputAccum.clear();
    return text;
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
    m_Resized = InFlag;
}

bool Window::WasWindowResized() const {
    return m_Resized;
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

bool Window::IsMinimized() const {
    return glfwGetWindowAttrib(m_Window, GLFW_ICONIFIED) == GLFW_TRUE;
}

void Window::Close() {
    glfwSetWindowShouldClose(m_Window, GLFW_TRUE);
}

void Window::Show() {
    glfwShowWindow(m_Window);
}

void Window::Hide() {
    glfwHideWindow(m_Window);
}

bool Window::IsVisible() const {
    return glfwGetWindowAttrib(m_Window, GLFW_VISIBLE) == GLFW_TRUE;
}

bool Window::IsFocused() const {
    return glfwGetWindowAttrib(m_Window, GLFW_FOCUSED) == GLFW_TRUE;
}

void Window::Focus() {
    glfwFocusWindow(m_Window);
}

Vec2 Window::GetPosition() const {
    int x, y;
    glfwGetWindowPos(m_Window, &x, &y);
    return Vec2((float)x, (float)y);
}

void Window::SetPosition(const Vec2& InScreenPos) {
    glfwSetWindowPos(m_Window, (int)InScreenPos.x, (int)InScreenPos.y);
}

Vec2 Window::GetCursorPosition() const {
    double x, y;
    glfwGetCursorPos(m_Window, &x, &y);
    return Vec2((float)x, (float)y);
}

bool Window::IsMouseButtonDown(int32_t InButton) const {
    return glfwGetMouseButton(m_Window, InButton) == GLFW_PRESS;
}

Vec2 Window::ConsumeScrollDelta() {
    const Vec2 delta = m_ScrollAccum;
    m_ScrollAccum = Vec2(0.0f);
    return delta;
}

Vec2 Window::ConsumeGlobalScrollDelta() {
    const Vec2 delta = s_GlobalScrollAccum;
    s_GlobalScrollAccum = Vec2(0.0f);
    return delta;
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

// Standard cursors are created once per process and shared by every window; GLFW frees them in
// glfwTerminate. A null cursor (unsupported shape on this platform) falls back to the arrow.
static GLFWcursor* GetStandardCursor(CursorIcon InIcon) {
    static GLFWcursor* s_Cursors[16] = {};
    const int index = (int)InIcon;
    if (index <= 0 || index >= 16) {
        return nullptr;  // Arrow: glfwSetCursor(nullptr) restores the default
    }
    if (!s_Cursors[index]) {
        int shape = GLFW_ARROW_CURSOR;
        switch (InIcon) {
            case CursorIcon::Text: shape = GLFW_IBEAM_CURSOR; break;
            case CursorIcon::Hand: shape = GLFW_POINTING_HAND_CURSOR; break;
            case CursorIcon::Crosshair: shape = GLFW_CROSSHAIR_CURSOR; break;
            case CursorIcon::ResizeH: shape = GLFW_RESIZE_EW_CURSOR; break;
            case CursorIcon::ResizeV: shape = GLFW_RESIZE_NS_CURSOR; break;
            case CursorIcon::ResizeNWSE: shape = GLFW_RESIZE_NWSE_CURSOR; break;
            case CursorIcon::ResizeNESW: shape = GLFW_RESIZE_NESW_CURSOR; break;
            case CursorIcon::ResizeAll: shape = GLFW_RESIZE_ALL_CURSOR; break;
            case CursorIcon::NotAllowed: shape = GLFW_NOT_ALLOWED_CURSOR; break;
            default: break;
        }
        s_Cursors[index] = glfwCreateStandardCursor(shape);
    }
    return s_Cursors[index];
}

void Window::SetCursorIcon(CursorIcon InIcon) {
    if (m_CursorIcon == InIcon) {
        return;
    }
    m_CursorIcon = InIcon;
    glfwSetCursor(m_Window, GetStandardCursor(InIcon));
}

SharedObjectPtr<Window> Window::Create(const WindowParams& InParams) {
    return SharedObjectPtr<Window>(new Window(InParams));
}

Window* Window::GetInstance() {
    return s_Instance;
}

Window* Window::GetFocusedWindow() {
    for (Window* window : s_AllWindows) {
        if (window->m_Window && window->IsFocused()) {
            return window;
        }
    }
    return nullptr;
}

GLFWwindow* Window::GetGLFWwindow() {
    if (Window* focused = GetFocusedWindow()) {
        return focused->m_Window;
    }
    return s_Instance ? s_Instance->m_Window : nullptr;
}

#include "Window.h"
#include "Core/Log.h"

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

    m_Params = InParams;
    // TODO: Window SHARED CONTEXT
    m_Window = glfwCreateWindow(m_Params.Width, m_Params.Height, m_Params.Title.c_str(), nullptr, nullptr);
    if (!m_Window) {
        AE_ERROR("Failed to create GLFW window");
        glfwTerminate();
        return;
    }
    glfwSetWindowSizeCallback(m_Window, OnWindowResized);
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

SharedObjectPtr<Window> Window::Create(const WindowParams& InParams) {
    return SharedObjectPtr<Window>(new Window(InParams));
}

Window* Window::GetInstance() {
    return s_Instance;
}

GLFWwindow* Window::GetGLFWwindow() {
    return Window::GetInstance()->m_Window;
}
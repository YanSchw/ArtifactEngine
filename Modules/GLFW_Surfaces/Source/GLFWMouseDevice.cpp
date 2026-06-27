#include "GLFWMouseDevice.h"
#include "Window.h"

#include <GLFW/glfw3.h>

// GLFW only reports scroll through a callback, so accumulate it here and drain it each Tick.
static double s_ScrollX = 0.0;
static double s_ScrollY = 0.0;
static void OnScroll(GLFWwindow* InWindow, double InOffsetX, double InOffsetY) {
    s_ScrollX += InOffsetX;
    s_ScrollY += InOffsetY;
}

GLFWMouseDevice::GLFWMouseDevice() {
    glfwSetScrollCallback(Window::GetGLFWwindow(), OnScroll);
}

void GLFWMouseDevice::Tick() {
    Super::Tick();

    for (auto& [button, pressed] : m_Buttons) {
        auto state = glfwGetMouseButton(Window::GetGLFWwindow(), static_cast<int32_t>(button));
        pressed = (bool)(state == GLFW_PRESS);
    }

    double x, y;
    glfwGetCursorPos(Window::GetGLFWwindow(), &x, &y);
    m_Position = {(float)x, (float)y};

    m_ScrollDelta = {(float)s_ScrollX, (float)s_ScrollY};
    s_ScrollX = 0.0;
    s_ScrollY = 0.0;
}

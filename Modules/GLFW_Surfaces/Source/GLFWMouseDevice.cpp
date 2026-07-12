#include "GLFWMouseDevice.h"
#include "Window.h"

#include <GLFW/glfw3.h>

GLFWMouseDevice::GLFWMouseDevice() {
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

    m_ScrollDelta = Window::ConsumeGlobalScrollDelta();
}

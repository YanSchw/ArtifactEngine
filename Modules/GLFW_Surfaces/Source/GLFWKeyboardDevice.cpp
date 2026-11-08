#include "GLFWKeyboardDevice.h"
#include "Window.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

void GLFWKeyboardDevice::Tick() {
    Super::Tick();

    for (auto& [key, pressed] : m_Keys) {
        auto state = glfwGetKey(Window::GetGLFWwindow(), static_cast<int32_t>(key));
        pressed = (bool)(state == GLFW_PRESS || state == GLFW_REPEAT);
    }
}
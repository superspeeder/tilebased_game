#include "game/window.hpp"

namespace game {
    Window::Window() {
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        m_Window = glfwCreateWindow(800, 600, "Hello!", nullptr, nullptr);
    }

    Window::~Window() {
        glfwDestroyWindow(m_Window);
    }

    bool Window::shouldClose() const {
        return glfwWindowShouldClose(m_Window);
    }

    vk::Extent2D Window::getExtent() const {
        int w, h;
        glfwGetFramebufferSize(m_Window, &w, &h);
        return {static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
    }
} // namespace game

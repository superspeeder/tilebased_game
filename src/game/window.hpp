#pragma once

#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

namespace game {
    class Window {
    public:
        Window();
        ~Window();

        [[nodiscard]] bool shouldClose() const;

        inline GLFWwindow* window() const {
            return m_Window;
        };

        vk::Extent2D getExtent() const;

    private:
        GLFWwindow* m_Window;

    };
}
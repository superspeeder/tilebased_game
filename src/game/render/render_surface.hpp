//
// Created by andy on 6/5/2025.
//

#pragma once

#include <optional>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "game/window.hpp"
#include "game/render/render_device.hpp"

namespace game {

    class RenderSurface {
    public:
        RenderSurface(std::shared_ptr<Window> window, std::shared_ptr<RenderDevice> renderDevice);
        ~RenderSurface();

        void createSwapchain();

        inline const vk::raii::SurfaceKHR &surface() const noexcept { return m_Surface; }

        inline const vk::raii::SwapchainKHR &swapchain() const noexcept { return m_Swapchain; }

        inline const std::vector<vk::Image> &images() const noexcept { return m_Images; }

        inline vk::Format format() const noexcept { return m_Format; }

        inline vk::ColorSpaceKHR colorSpace() const noexcept { return m_ColorSpace; }

        inline vk::PresentModeKHR presentMode() const noexcept { return m_PresentMode; }

        inline vk::Extent2D extent() const noexcept { return m_Extent; }

        std::optional<std::tuple<vk::Image, uint32_t>> acquireNextImage(vk::Semaphore semaphore);

        bool checkRebuildSwapchain();
        void present(uint32_t index, vk::Semaphore wait);

    private:
        std::shared_ptr<Window>       m_Window;
        std::shared_ptr<RenderDevice> m_RenderDevice;

        vk::raii::SurfaceKHR   m_Surface{nullptr};
        vk::raii::SwapchainKHR m_Swapchain{nullptr};

        std::vector<vk::Image> m_Images;

        vk::Format         m_Format;
        vk::ColorSpaceKHR  m_ColorSpace;
        vk::PresentModeKHR m_PresentMode;
        vk::Extent2D       m_Extent;
        bool               m_NeedsRebuild = false;
    };

} // game

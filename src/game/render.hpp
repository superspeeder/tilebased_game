#pragma once

#include <optional>
#include <functional>
#include <deque>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace game {
    class Window;

    class RenderDevice {
      public:
        RenderDevice();
        ~RenderDevice();

        inline const vk::raii::Instance &instance() const noexcept { return m_Instance; };

        inline const vk::raii::PhysicalDevice &physicalDevice() const noexcept { return m_PhysicalDevice; };

        inline const vk::raii::Device &device() const noexcept { return m_Device; };

        inline uint32_t mainFamily() const noexcept { return m_MainFamily; };

        inline uint32_t presentFamily() const noexcept { return m_PresentFamily; };

        inline const std::optional<uint32_t> &exclusiveTransferFamily() const noexcept { return m_ExclusiveTransferFamily; };

        inline const vk::raii::Queue &mainQueue() const noexcept { return m_MainQueue; };

        inline const vk::raii::Queue &presentQueue() const noexcept { return m_PresentQueue; };

        inline const std::optional<vk::raii::Queue> &exclusiveTransferQueue() const noexcept { return m_ExclusiveTransferQueue; };

      private:
        vk::raii::Context        m_Context;
        vk::raii::Instance       m_Instance{nullptr};
        vk::raii::PhysicalDevice m_PhysicalDevice{nullptr};
        vk::raii::Device         m_Device{nullptr};

        uint32_t                m_MainFamily              = UINT32_MAX;
        uint32_t                m_PresentFamily           = UINT32_MAX;
        std::optional<uint32_t> m_ExclusiveTransferFamily = std::nullopt;

        vk::raii::Queue                m_MainQueue{nullptr};
        vk::raii::Queue                m_PresentQueue{nullptr};
        std::optional<vk::raii::Queue> m_ExclusiveTransferQueue = std::nullopt;
    };

    class RenderSurface {
      public:
        RenderSurface(std::shared_ptr<Window> window, std::shared_ptr<RenderDevice> renderDevice);
        ~RenderSurface();

        void createSwapchain();

        inline const vk::raii::SurfaceKHR &surface() const noexcept {
            return m_Surface;
        }

        inline const vk::raii::SwapchainKHR &swapchain() const noexcept {
            return m_Swapchain;
        }

        inline const std::vector<vk::Image> &images() const noexcept {
            return m_Images;
        }

        inline vk::Format format() const noexcept {
            return m_Format;
        }

        inline vk::ColorSpaceKHR colorSpace() const noexcept {
            return m_ColorSpace;
        }

        inline vk::PresentModeKHR presentMode() const noexcept {
            return m_PresentMode;
        }

        inline vk::Extent2D extent() const noexcept {
            return m_Extent;
        }



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
    };

    struct PooledCommandBuffer {
        vk::raii::CommandBuffer commandBuffer;
        vk::raii::Fence availabilityFence;
    };

    class SingleTimeCommands {
    public:
        static constexpr std::size_t INITIAL_CAPACITY = 16;

        SingleTimeCommands(std::shared_ptr<RenderDevice> renderDevice, uint32_t family, const vk::raii::Queue& queue);
        ~SingleTimeCommands() = default;

        void pollInUse();

        void runCommands(const std::function<void(const vk::raii::CommandBuffer& cmd)>, std::optional<vk::SemaphoreSubmitInfo> waitSemaphore, std::optional<vk::SemaphoreSubmitInfo> signalSemaphore);

        std::size_t acquireCommandBuffer();

        void expandCapacity();
    private:
        std::shared_ptr<RenderDevice> m_RenderDevice;
        vk::raii::CommandPool m_CommandPool;

        uint32_t m_Family;
        vk::raii::Queue m_Queue;
        
        std::deque<std::size_t> m_Available;
        std::vector<std::size_t> m_InUse;
        std::vector<PooledCommandBuffer> m_CommandBuffers;

    };
} // namespace game

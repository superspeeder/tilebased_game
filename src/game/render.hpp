#pragma once

#include "game/utils.hpp"


#include <deque>
#include <functional>
#include <optional>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace game {
    class Window;
    class SingleTimeCommands;

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

        template <std::ranges::contiguous_range R>
        void resetFences(R &&fences) const {
            m_Device.resetFences(std::forward<R>(fences));
        }

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

    struct PooledCommandBuffer {
        vk::raii::CommandBuffer commandBuffer;
        vk::raii::Fence         availabilityFence;
    };

    class SingleTimeCommands {
      public:
        static constexpr std::size_t INITIAL_CAPACITY = 16;

        SingleTimeCommands(std::shared_ptr<RenderDevice> renderDevice, uint32_t family, const vk::raii::Queue &queue);
        ~SingleTimeCommands() = default;

        // this should be called regularly (probably no more than once per frame though) in order to not just allocate infinite command buffers.
        void pollInUse();

        void runCommands(
            const std::function<void(const vk::raii::CommandBuffer &cmd)> &f, std::optional<vk::SemaphoreSubmitInfo> waitSemaphore,
            std::optional<vk::SemaphoreSubmitInfo> signalSemaphore
        );

        std::size_t acquireCommandBuffer();

        void expandCapacity();

      private:
        std::shared_ptr<RenderDevice> m_RenderDevice;
        vk::raii::CommandPool         m_CommandPool;

        uint32_t        m_Family;
        vk::raii::Queue m_Queue;

        std::deque<std::size_t>          m_Available;
        std::vector<std::size_t>         m_InUse;
        std::vector<PooledCommandBuffer> m_CommandBuffers;
    };

    class RenderSystem {
      public:
        RenderSystem(std::shared_ptr<RenderDevice> renderDevice, std::shared_ptr<RenderSurface> renderSurface);

        [[nodiscard]] std::shared_ptr<RenderDevice> renderDevice() const { return m_RenderDevice; }

        [[nodiscard]] std::shared_ptr<RenderSurface> renderSurface() const { return m_RenderSurface; }

        [[nodiscard]] std::shared_ptr<SingleTimeCommands> stc() const { return m_STC; }

        bool checkRebuildSwapchain() const;

      private:
        std::shared_ptr<RenderDevice>       m_RenderDevice;
        std::shared_ptr<RenderSurface>      m_RenderSurface;
        std::shared_ptr<SingleTimeCommands> m_STC;
    };

    struct ImageProperties {
        vk::Format   format;
        vk::Extent2D extent;
    };

    struct FrameSyncObjects {
        vk::raii::Fence     inFlightFence;
        vk::raii::Semaphore imageAvailableSemaphore;
        vk::raii::Semaphore renderFinishedSemaphore;

        explicit FrameSyncObjects(const std::shared_ptr<RenderSystem> &renderSystem)
            : inFlightFence(renderSystem->renderDevice()->device(), vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled)),
              imageAvailableSemaphore(renderSystem->renderDevice()->device(), vk::SemaphoreCreateInfo()),
              renderFinishedSemaphore(renderSystem->renderDevice()->device(), vk::SemaphoreCreateInfo()) {}
    };

    /**
     * @brief Utility to manage frames
     * @tparam F Frame based resource type. This is good for things like descriptor sets.
     * @tparam I Image based resource type (i.e. making image views for images. These get refreshed when the target RenderSurface changes images).
     */
    template <typename F, typename I>
    class FrameManager {
      public:
        constexpr static std::size_t MAX_FRAMES_IN_FLIGHT = 2;

        FrameManager(
            std::shared_ptr<RenderSystem> renderSystem, const std::function<F(std::size_t, const std::shared_ptr<RenderSystem> &)> frameResourceFunction,
            std::function<I(std::size_t, const std::shared_ptr<RenderSystem> &, const ImageProperties &, vk::Image)> imageResourceFunction
        )
            : m_RenderSystem(std::move(renderSystem)), m_FrameResources(array_from_fn<F, MAX_FRAMES_IN_FLIGHT>([&](std::size_t i) -> F { return frameResourceFunction(i, m_RenderSystem); })),
              m_FrameSyncObjects(array_from_fn<FrameSyncObjects, MAX_FRAMES_IN_FLIGHT>([&](std::size_t i) -> FrameSyncObjects { return FrameSyncObjects(m_RenderSystem); })),
              m_ImageResourceFunction(std::move(imageResourceFunction)),
              m_CommandPool(
                  m_RenderSystem->renderDevice()->device(),
                  vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_RenderSystem->renderDevice()->mainFamily())
              ),
              m_CommandBuffers(m_RenderSystem->renderDevice()->device(), vk::CommandBufferAllocateInfo(m_CommandPool, vk::CommandBufferLevel::ePrimary, MAX_FRAMES_IN_FLIGHT)) {
            generateImageResources();
        }

        void generateImageResources() {
            m_ImageResources.clear();
            ImageProperties iprop{m_RenderSystem->renderSurface()->format(), m_RenderSystem->renderSurface()->extent()};
            m_ImageResources.reserve(m_RenderSystem->renderSurface()->images().size());
            for (std::size_t i = 0; i < m_RenderSystem->renderSurface()->images().size(); ++i) {
                m_ImageResources.push_back(std::move(m_ImageResourceFunction(i, m_RenderSystem, iprop, m_RenderSystem->renderSurface()->images()[i])));
            }
        }

        void renderFrame(const std::function<void(const vk::raii::CommandBuffer &, const F &, const I &, const ImageProperties &, vk::Image)> &f) {
            if (m_RenderSystem->checkRebuildSwapchain()) {
                generateImageResources();
            }

            const auto           &fso = m_FrameSyncObjects[m_CurrentFrame];
            [[maybe_unused]] auto _   = m_RenderSystem->renderDevice()->device().waitForFences(*fso.inFlightFence, true, UINT64_MAX);
            if (const auto curImage = m_RenderSystem->renderSurface()->acquireNextImage(fso.imageAvailableSemaphore); curImage.has_value()) {
                m_RenderSystem->renderDevice()->device().resetFences(*fso.inFlightFence);
                const auto &[image, index] = curImage.value();
                m_CurrentImageIndex        = index;

                ImageProperties imageProperties{m_RenderSystem->renderSurface()->format(), m_RenderSystem->renderSurface()->extent()};

                const auto &cmd = m_CommandBuffers[m_CurrentFrame];
                cmd.reset();
                cmd.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
                f(cmd, m_FrameResources[m_CurrentFrame], m_ImageResources[m_CurrentImageIndex], imageProperties, image);
                cmd.end();

                const vk::PipelineStageFlags mask = vk::PipelineStageFlagBits::eTopOfPipe;

                vk::SubmitInfo submitInfo{};
                submitInfo.setCommandBuffers(*cmd);
                submitInfo.setWaitSemaphores(*fso.imageAvailableSemaphore);
                submitInfo.setSignalSemaphores(*fso.renderFinishedSemaphore);
                submitInfo.setWaitDstStageMask(mask);
                m_RenderSystem->renderDevice()->mainQueue().submit(submitInfo, fso.inFlightFence);

                m_RenderSystem->renderSurface()->present(index, fso.renderFinishedSemaphore);

                m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
            }
        }

      private:
        std::shared_ptr<RenderSystem>       m_RenderSystem;
        std::array<F, MAX_FRAMES_IN_FLIGHT> m_FrameResources;
        std::vector<I>                      m_ImageResources;

        std::array<FrameSyncObjects, MAX_FRAMES_IN_FLIGHT> m_FrameSyncObjects;

        std::function<I(std::size_t, const std::shared_ptr<RenderSystem> &, const ImageProperties &, vk::Image)> m_ImageResourceFunction;

        vk::raii::CommandPool    m_CommandPool;
        vk::raii::CommandBuffers m_CommandBuffers;

        uint32_t m_CurrentFrame      = 0;
        uint32_t m_CurrentImageIndex = 0;
    };
} // namespace game

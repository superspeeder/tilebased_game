//
// Created by andy on 6/5/2025.
//

#pragma once

#include <functional>
#include <memory>
#include <vulkan/vulkan_raii.hpp>

#include "game/utils.hpp"
#include "game/render/render.hpp"
#include "game/render/render_system.hpp"

namespace game {
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
            : m_RenderSystem(std::move(renderSystem)),
              m_FrameResources(array_from_fn<F, MAX_FRAMES_IN_FLIGHT>([&](std::size_t i) -> F { return frameResourceFunction(i, m_RenderSystem); })),
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

                constexpr vk::PipelineStageFlags mask = vk::PipelineStageFlagBits::eTopOfPipe;

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

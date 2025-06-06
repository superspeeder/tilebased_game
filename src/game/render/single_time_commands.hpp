//
// Created by andy on 6/5/2025.
//

#pragma once

#include <functional>
#include <deque>
#include <vector>
#include <memory>

#include "game/render/render_device.hpp"

namespace game {
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

} // namespace game

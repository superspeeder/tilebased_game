//
// Created by andy on 6/5/2025.
//

#include "single_time_commands.hpp"

namespace game {
    SingleTimeCommands::SingleTimeCommands(std::shared_ptr<RenderDevice> renderDevice, uint32_t family, const vk::raii::Queue &queue)
        : m_RenderDevice(std::move(renderDevice)), m_CommandPool(m_RenderDevice->device(), vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, family)),
          m_Family(family), m_Queue(queue) {

        m_CommandBuffers.reserve(INITIAL_CAPACITY);
        auto commandBuffers = vk::raii::CommandBuffers(m_RenderDevice->device(), vk::CommandBufferAllocateInfo(m_CommandPool, vk::CommandBufferLevel::ePrimary, INITIAL_CAPACITY));
        std::size_t i       = 0;
        for (auto &&cmd : commandBuffers) {
            m_CommandBuffers.emplace_back(std::move(cmd), vk::raii::Fence(m_RenderDevice->device(), vk::FenceCreateInfo()));
            m_Available.emplace_back(i++);
        }
    }

    void SingleTimeCommands::pollInUse() {
        const auto it = std::remove_if(m_InUse.begin(), m_InUse.end(), [&](const std::size_t &i) -> bool {
            return m_CommandBuffers[i].availabilityFence.getStatus() == vk::Result::eSuccess;
        });

        if (it == m_InUse.end())
            return; // nothing to do

        std::vector<vk::Fence> fencesToReset;
        fencesToReset.reserve(std::distance(it, m_InUse.end()));
        for (const auto &i : std::ranges::subrange(it, m_InUse.end())) {
            fencesToReset.push_back(m_CommandBuffers[i].availabilityFence);
        }

        m_RenderDevice->resetFences(fencesToReset);

        m_Available.insert(m_Available.end(), it, m_InUse.end());
        m_InUse.erase(it, m_InUse.end());
    }

    void SingleTimeCommands::runCommands(
        const std::function<void(const vk::raii::CommandBuffer &cmd)> &f, std::optional<vk::SemaphoreSubmitInfo> waitSemaphore,
        std::optional<vk::SemaphoreSubmitInfo> signalSemaphore
    ) {
        const auto commandBufferIndex                  = acquireCommandBuffer();
        const auto &[commandBuffer, availabilityFence] = m_CommandBuffers[commandBufferIndex];
        commandBuffer.reset();
        commandBuffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
        f(commandBuffer);
        commandBuffer.end();

        vk::SubmitInfo2 submitInfo{};
        if (waitSemaphore.has_value()) {
            submitInfo.setWaitSemaphoreInfos(waitSemaphore.value());
        }
        if (signalSemaphore.has_value()) {
            submitInfo.setSignalSemaphoreInfos(signalSemaphore.value());
        }

        vk::CommandBufferSubmitInfo cmdSubmit{};
        cmdSubmit.commandBuffer = commandBuffer;

        submitInfo.setCommandBufferInfos(cmdSubmit);

        m_Queue.submit2(submitInfo, availabilityFence);
    }

    std::size_t SingleTimeCommands::acquireCommandBuffer() {
        if (m_Available.empty())
            expandCapacity();
        std::size_t i = m_Available.front();
        m_Available.pop_front();
        return i;
    }

    void SingleTimeCommands::expandCapacity() {
        // double capacity
        auto commandBuffers = vk::raii::CommandBuffers(
            m_RenderDevice->device(), vk::CommandBufferAllocateInfo(m_CommandPool, vk::CommandBufferLevel::ePrimary, m_CommandBuffers.size())
        );
        std::size_t i = m_CommandBuffers.size();
        m_CommandBuffers.reserve(m_CommandBuffers.size() * 2);
        for (auto &&cmd : commandBuffers) {
            m_CommandBuffers.emplace_back(std::move(cmd), vk::raii::Fence(m_RenderDevice->device(), vk::FenceCreateInfo()));
            m_Available.emplace_back(i++);
        }
    }
} // namespace game

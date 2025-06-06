//
// Created by andy on 6/5/2025.
//

#include "render_system.hpp"

namespace game {
    RenderSystem::RenderSystem(std::shared_ptr<RenderDevice> renderDevice, std::shared_ptr<RenderSurface> renderSurface)
        : m_RenderDevice(std::move(renderDevice)), m_RenderSurface(std::move(renderSurface)) {
        m_STC       = std::make_shared<SingleTimeCommands>(m_RenderDevice, m_RenderDevice->mainFamily(), m_RenderDevice->mainQueue());
        m_Allocator = std::make_shared<Allocator>(m_RenderDevice);
    }

    bool RenderSystem::checkRebuildSwapchain() const {
        return m_RenderSurface->checkRebuildSwapchain();
    }
} // namespace game

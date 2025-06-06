//
// Created by andy on 6/5/2025.
//

#pragma once

#include "game/render/allocator.hpp"
#include "game/render/render_device.hpp"
#include "game/render/render_surface.hpp"
#include "game/render/single_time_commands.hpp"

namespace game {
    class RenderSystem {
      public:
        RenderSystem(std::shared_ptr<RenderDevice> renderDevice, std::shared_ptr<RenderSurface> renderSurface);

        [[nodiscard]] std::shared_ptr<RenderDevice> renderDevice() const { return m_RenderDevice; }

        [[nodiscard]] std::shared_ptr<RenderSurface> renderSurface() const { return m_RenderSurface; }

        [[nodiscard]] std::shared_ptr<SingleTimeCommands> stc() const { return m_STC; }

        [[nodiscard]] std::shared_ptr<Allocator> allocator() const { return m_Allocator; }

        bool checkRebuildSwapchain() const;

      private:
        std::shared_ptr<RenderDevice>       m_RenderDevice;
        std::shared_ptr<RenderSurface>      m_RenderSurface;
        std::shared_ptr<SingleTimeCommands> m_STC;
        std::shared_ptr<Allocator>          m_Allocator;
    };
} // namespace game

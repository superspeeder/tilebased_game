#include "game/game.hpp"
#include <iostream>

namespace game {
    libload::libload() {
        glfwInit();
    }

    libload::~libload() {
        glfwTerminate();
    }

    Game::Game() {
        m_Window        = std::make_shared<Window>();
        m_RenderDevice  = std::make_shared<RenderDevice>();
        m_RenderSurface = std::make_shared<RenderSurface>(m_Window, m_RenderDevice);
        m_RenderSystem  = std::make_shared<RenderSystem>(m_RenderDevice, m_RenderSurface);
        m_FrameManager  = std::make_shared<FrameManager<FrameResources, ImageResources>>(m_RenderSystem, &FrameResources::create, &ImageResources::create);

        std::cout << "Swapchain has " << m_RenderSurface->images().size() << " images" << std::endl;
    }

    Game::~Game() = default;

    void Game::run() {
        while (!m_Window->shouldClose()) {
            glfwPollEvents();
            m_FrameManager->renderFrame([&](const vk::raii::CommandBuffer &cmd, const FrameResources &frameResources, const ImageResources &imageResources,
                                            const ImageProperties &imageProperties, const vk::Image image) { render(cmd, frameResources, imageResources, imageProperties, image); }
            );
        }
    }

    void Game::render(
        const vk::raii::CommandBuffer &cmd, const FrameResources &frameResources, const ImageResources &imageResources, const ImageProperties &imageProperties,
        const vk::Image image
    ) {
        vk::ImageMemoryBarrier imb{};
        imb.image = image;
        imb.oldLayout = vk::ImageLayout::eUndefined;
        imb.newLayout = vk::ImageLayout::ePresentSrcKHR;
        imb.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, {}, {}, imb);
    }
} // namespace game

int main() {
    auto game = std::make_unique<game::Game>();
    game->run();
    return 0;
}

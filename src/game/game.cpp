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

        glfwSetWindowUserPointer(m_Window->window(), this);

        glfwSetWindowRefreshCallback(
            m_Window->window(),
            +[](GLFWwindow *window) {
                auto *game = static_cast<Game *>(glfwGetWindowUserPointer(window));
                game->frame();
            }
        );
    }

    Game::~Game() = default;

    void Game::frame() {
        if (glfwGetWindowAttrib(m_Window->window(), GLFW_ICONIFIED))
            return;

        m_FrameManager->renderFrame([&](const vk::raii::CommandBuffer &cmd, const FrameResources &frameResources, const ImageResources &imageResources,
                                        const ImageProperties &imageProperties, const vk::Image image) { render(cmd, frameResources, imageResources, imageProperties, image); });
    }

    void Game::run() {
        while (!m_Window->shouldClose()) {
            glfwPollEvents();
            frame();
        }

        m_RenderDevice->device().waitIdle();
    }

    void Game::render(
        const vk::raii::CommandBuffer &cmd, const FrameResources &frameResources, const ImageResources &imageResources, const ImageProperties &imageProperties,
        const vk::Image image
    ) {
        {
            vk::ImageMemoryBarrier imb{};
            imb.image            = image;
            imb.oldLayout        = vk::ImageLayout::eUndefined;
            imb.newLayout        = vk::ImageLayout::eColorAttachmentOptimal;
            imb.srcAccessMask    = vk::AccessFlagBits::eNone;
            imb.dstAccessMask    = vk::AccessFlagBits::eColorAttachmentWrite;
            imb.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, {}, {}, imb);
        }

        {
            vk::RenderingAttachmentInfo attachmentInfo{};
            attachmentInfo.clearValue  = vk::ClearColorValue(1.0f, 1.0f, 1.0f, 1.0f);
            attachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
            attachmentInfo.imageView   = imageResources.imageView;
            attachmentInfo.loadOp      = vk::AttachmentLoadOp::eClear;
            attachmentInfo.storeOp     = vk::AttachmentStoreOp::eStore;

            vk::RenderingInfo renderingInfo{};
            renderingInfo.setColorAttachments(attachmentInfo);
            renderingInfo.setLayerCount(1);
            renderingInfo.setRenderArea(vk::Rect2D({0, 0}, imageProperties.extent));

            cmd.beginRendering(renderingInfo);
            cmd.endRendering();
        }

        {
            vk::ImageMemoryBarrier imb{};
            imb.image            = image;
            imb.oldLayout        = vk::ImageLayout::eColorAttachmentOptimal;
            imb.newLayout        = vk::ImageLayout::ePresentSrcKHR;
            imb.srcAccessMask    = vk::AccessFlagBits::eColorAttachmentWrite;
            imb.dstAccessMask    = vk::AccessFlagBits::eNone;
            imb.subresourceRange = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, {}, {}, imb);
        }
    }
} // namespace game

int main() {
    auto game = std::make_unique<game::Game>();
    game->run();
    return 0;
}

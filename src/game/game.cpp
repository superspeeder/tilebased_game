#include "game/game.hpp"

#include "game/asset/asset_manager.hpp"

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
        m_AssetManager  = std::make_shared<AssetManager>(m_RenderSystem);

        m_Bundle = m_AssetManager->loadFromFile<AssetBundleLoader>("simple_bundle.json");

        m_Shader         = m_AssetManager->get<Shader>("shaders/sample_linked_shader.json");
        m_PipelineLayout = m_AssetManager->get<PipelineLayout>("render/sample_pipeline_layout.json");

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

        if (const auto extent = m_Window->getExtent(); extent.width > 0 && extent.height > 0) {
            m_FrameManager->renderFrame([&](const vk::raii::CommandBuffer &cmd, const FrameResources &frameResources, const ImageResources &imageResources,
                                            const ImageProperties &imageProperties, const vk::Image image) { render(cmd, frameResources, imageResources, imageProperties, image); }
            );
        }
    }

    void Game::run() {
        m_AssetManager->beginDeletionThread();

        int frames = 0;

        while (!m_Window->shouldClose()) {
            glfwPollEvents();
            m_AssetManager->startDeletionCycle();
            frame();
            m_AssetManager->deleteWaitingAssets();

            if (frames < 20000) {
                if (++frames == 20000) {
//                    m_UnlinkedShaderFrag.reset();
//                    m_UnlinkedShaderVert.reset();
                    std::cout << "trigger" << std::endl;
                }
            }
        }

        // request the thread to stop, then wait for the vulkan device to be idle, then join the deletion thread (no reason these two need to be sequenced so we just do this).
        m_AssetManager->endDeletionThread();
        m_RenderDevice->device().waitIdle();
        m_AssetManager->finalEndDeletionThread();
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
            renderPass(cmd, frameResources, imageResources, imageProperties, image);
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

    void setDefaultState(const vk::raii::CommandBuffer &cmd, const ImageProperties &imageProperties, bool enableDepthTest = false) {
        const auto scissor  = vk::Rect2D({0, 0}, imageProperties.extent);
        const auto viewport = vk::Viewport{0.0f, 0.0f, static_cast<float>(imageProperties.extent.width), static_cast<float>(imageProperties.extent.height), 0.0, 1.0};

        cmd.setViewportWithCount(viewport);
        cmd.setScissorWithCount(scissor);
        cmd.setRasterizerDiscardEnable(false);

        cmd.setVertexInputEXT({}, {});
        cmd.setPrimitiveTopology(vk::PrimitiveTopology::eTriangleList);
        cmd.setPrimitiveRestartEnable(false);

        cmd.setPatchControlPointsEXT(1);
        cmd.setTessellationDomainOriginEXT(vk::TessellationDomainOrigin::eLowerLeft);

        cmd.setRasterizationSamplesEXT(vk::SampleCountFlagBits::e1);
        cmd.setSampleMaskEXT(vk::SampleCountFlagBits::e1, ~0U);
        cmd.setAlphaToCoverageEnableEXT(false);
        cmd.setAlphaToOneEnableEXT(false);
        cmd.setPolygonModeEXT(vk::PolygonMode::eFill);
        cmd.setLineWidth(1.0f);
        cmd.setCullMode(vk::CullModeFlagBits::eBack);
        cmd.setFrontFace(vk::FrontFace::eClockwise);

        cmd.setDepthTestEnable(enableDepthTest);
        cmd.setDepthWriteEnable(true);
        cmd.setDepthCompareOp(vk::CompareOp::eLess);
        cmd.setDepthBoundsTestEnable(false);
        cmd.setDepthBounds(0.0f, 1.0f);
        cmd.setDepthBiasEnable(false);
        cmd.setDepthBias(0.0f, 0.0f, 0.0f);
        cmd.setDepthClampEnableEXT(false);
        cmd.setStencilTestEnable(false);
        cmd.setStencilOp(
            vk::StencilFaceFlagBits::eFrontAndBack, vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::CompareOp::eNever
        ); // this op will do actually nothing so if we want to do anything stencil-test wise we should change this (not here though, this makes sense here)
        cmd.setStencilCompareMask(
            vk::StencilFaceFlagBits::eFrontAndBack, 0
        ); // this makes sure we don't write to any stencil bits from either side if you enable stencil test (and is required to be changed to use the stencil test for something).
        cmd.setStencilWriteMask(vk::StencilFaceFlagBits::eFrontAndBack, 0);
        cmd.setStencilReference(vk::StencilFaceFlagBits::eFrontAndBack, 0);

        cmd.setLogicOpEnableEXT(false);
        cmd.setLogicOpEXT(vk::LogicOp::eCopy);

        constexpr float blendConstants[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        cmd.setColorBlendEnableEXT(0, true);
        cmd.setColorWriteMaskEXT(0, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
        cmd.setColorBlendEquationEXT(
            0,
            vk::ColorBlendEquationEXT(
                vk::BlendFactor::eSrcAlpha, vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd, vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd
            )
        );
        cmd.setBlendConstants(blendConstants);

        cmd.bindShadersEXT(
            {vk::ShaderStageFlagBits::eVertex, vk::ShaderStageFlagBits::eFragment, vk::ShaderStageFlagBits::eGeometry, vk::ShaderStageFlagBits::eTessellationControl,
             vk::ShaderStageFlagBits::eTessellationEvaluation},
            {nullptr, nullptr, nullptr, nullptr, nullptr}
        );
    }

    struct PC {
        float x, y, time;
    };

    void Game::renderPass(
        const vk::raii::CommandBuffer &cmd, const FrameResources &frameResources, const ImageResources &imageResources, const ImageProperties &imageProperties, vk::Image image
    ) {
        double t = glfwGetTime();
        PC     pc{};
        pc.x    = 0.25 * cos(t);
        pc.y    = 0.25 * sin(t);
        pc.time = t;

        setDefaultState(cmd, imageProperties);
        m_Shader->bind(cmd);
        m_PipelineLayout->pushConstants<PC>(cmd, vk::ShaderStageFlagBits::eVertex, 0, pc);
        cmd.draw(3, 1, 0, 0);
    }
} // namespace game

int main() {
    const auto game = std::make_unique<game::Game>();
    game->run();
    return 0;
}

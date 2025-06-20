#pragma once

#include <GLFW/glfw3.h>
#include <memory>

#include "game/asset/asset_bundle.hpp"
#include "game/render/frame_manager.hpp"
#include "game/render/pipeline_layout.hpp"
#include "game/render/render_system.hpp"
#include "game/render/shader_object.hpp"
#include "game/window.hpp"

namespace game {
    struct libload {
        libload();
        ~libload();
    };

    struct FrameResources {
        static FrameResources create(std::size_t index, const std::shared_ptr<RenderSystem> &renderSystem) { return FrameResources{}; }
    };

    struct ImageResources {
        vk::raii::ImageView imageView;

        static ImageResources create(std::size_t index, const std::shared_ptr<RenderSystem> &renderSystem, const ImageProperties &imageProperties, const vk::Image image) {
            return ImageResources{vk::raii::ImageView(
                renderSystem->renderDevice()->device(),
                vk::ImageViewCreateInfo(
                    {}, image, vk::ImageViewType::e2D, imageProperties.format,
                    vk::ComponentMapping{vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA},
                    vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
                )
            )};
        }
    };

    class Game {
      public:
        Game();
        ~Game();

        void frame();
        void run();

        void render(
            const vk::raii::CommandBuffer &cmd, const FrameResources &frameResources, const ImageResources &imageResources, const ImageProperties &imageProperties, vk::Image image
        );

        void renderPass(
            const vk::raii::CommandBuffer &cmd, const FrameResources &frameResources, const ImageResources &imageResources, const ImageProperties &imageProperties, vk::Image image
        );

      private:
        libload                                                       _libload{};
        std::shared_ptr<Window>                                       m_Window;
        std::shared_ptr<RenderDevice>                                 m_RenderDevice;
        std::shared_ptr<RenderSurface>                                m_RenderSurface;
        std::shared_ptr<RenderSystem>                                 m_RenderSystem;
        std::shared_ptr<FrameManager<FrameResources, ImageResources>> m_FrameManager;
        std::shared_ptr<AssetManager>                                 m_AssetManager;

        AssetRef<AssetBundle> m_Bundle;

        // TODO: some kind of system to let me connect pipeline layouts to shaders
        AssetRef<Shader> m_Shader;
        AssetRef<PipelineLayout> m_PipelineLayout;
    };
} // namespace game

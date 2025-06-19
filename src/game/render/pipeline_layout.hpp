//
// Created by andy on 6/16/2025.
//

#pragma once
#include "game/asset/asset.hpp"
#include "game/asset/asset_loader.hpp"

#include <vulkan/vulkan_raii.hpp>

namespace game {

    class PipelineLayout : public Asset<PipelineLayout> {
      public:
        PipelineLayout(
            const std::shared_ptr<RenderDevice> &renderDevice, const std::vector<vk::PushConstantRange> &pushConstantRanges,
            const std::vector<vk::DescriptorSetLayout> &descriptorSetLayouts, asset_id_t id, const std::string &name
        );

        template <typename T>
        void pushConstants(const vk::raii::CommandBuffer &cmd, const vk::ShaderStageFlags stageFlags, const uint32_t offset, const vk::ArrayProxy<T> &values) {
            cmd.pushConstants<T>(m_PipelineLayout, stageFlags, offset, values);
        };


      private:
        std::shared_ptr<RenderDevice> m_RenderDevice;
        vk::raii::PipelineLayout      m_PipelineLayout;
    };

    class PipelineLayoutLoader : public JsonAssetLoader<PipelineLayout, std::nullptr_t> {
      public:
        PipelineLayout *load(
            const nlohmann::json &json, const std::nullptr_t &options, asset_id_t id, const std::string &name, const AssetLoaderContext &loaderContext
        ) const override;
    };

} // namespace game

//
// Created by andy on 6/16/2025.
//

#include "pipeline_layout.hpp"
#include "game/asset/common_parse.hpp"

namespace game {
    PipelineLayout::PipelineLayout(
        const std::shared_ptr<RenderDevice> &renderDevice, const std::vector<vk::PushConstantRange> &pushConstantRanges,
        const std::vector<vk::DescriptorSetLayout> &descriptorSetLayouts, asset_id_t id, const std::string &name
    )
        : Asset(id, name), m_RenderDevice(renderDevice), m_PipelineLayout(m_RenderDevice->device(), vk::PipelineLayoutCreateInfo({}, descriptorSetLayouts, pushConstantRanges)) {}

    PipelineLayout *PipelineLayoutLoader::load(
        const nlohmann::json &json, const std::nullptr_t &options, asset_id_t id, const std::string &name, const AssetLoaderContext &loaderContext
    ) const {
        // TODO: descriptor set layouts (waiting on asset resolver)

        std::vector<vk::PushConstantRange> pcrs;
        for (const auto &pcrArr = json["push_constant_ranges"]; const auto &pcr : pcrArr) {
            pcrs.push_back(parsePcrJson(pcr));
        }

        return new PipelineLayout(loaderContext.renderSystem->renderDevice(), pcrs, {}, id, name);
    }
} // namespace game

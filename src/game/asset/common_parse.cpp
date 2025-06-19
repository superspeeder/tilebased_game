//
// Created by andy on 6/16/2025.
//
#include "common_parse.hpp"

namespace game {
    vk::ShaderStageFlagBits parseStageBit(const std::string &s) {
        if (s == "vertex") {
            return vk::ShaderStageFlagBits::eVertex;
        }
        if (s == "fragment") {
            return vk::ShaderStageFlagBits::eFragment;
        }
        if (s == "geometry") {
            return vk::ShaderStageFlagBits::eGeometry;
        }
        if (s == "tess-control") {
            return vk::ShaderStageFlagBits::eTessellationControl;
        }
        if (s == "tess-evaluation") {
            return vk::ShaderStageFlagBits::eTessellationEvaluation;
        }
        if (s == "compute") {
            return vk::ShaderStageFlagBits::eCompute;
        }

        throw std::invalid_argument("Failed to parse shader stage name '" + s + "'");
    }

    vk::PushConstantRange parsePcrJson(const nlohmann::json& json) {
        /**
         * {
         *   stages: ["vertex",...],
         *   offset: 0,
         *   size: 4
         * }
         */

        const auto stages = json["stages"];
        if (!stages.is_array()) {
            throw std::invalid_argument("Failed to parse push constant range json: stages must be an array");
        }

        vk::ShaderStageFlags stageFlags{};
        for (const auto& stage : stages) {
            stageFlags |= parseStageBit(stage);
        }

        return vk::PushConstantRange(stageFlags, json["offset"].get<uint32_t>(), json["size"].get<uint32_t>());
    }
}
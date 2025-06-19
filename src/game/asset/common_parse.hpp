//
// Created by andy on 6/16/2025.
//

#pragma once

#include <nlohmann/json.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace game {
    vk::ShaderStageFlagBits parseStageBit(const std::string &s);
    vk::PushConstantRange parsePcrJson(const nlohmann::json &json);
}

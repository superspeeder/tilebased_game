//
// Created by andy on 6/5/2025.
//

#include "shader_object.hpp"

namespace game {
    Shader::Shader(const std::shared_ptr<RenderDevice> &renderDevice, const std::vector<vk::ShaderCreateInfoEXT> &createInfos)
        : Asset<Shader>(0), m_Shaders(renderDevice->device(), createInfos) {
    }

    void Shader::bind(const vk::raii::CommandBuffer &cmd) const {
        cmd.bindShadersEXT(m_Stages, m_ShadersI);
    }
} // namespace game

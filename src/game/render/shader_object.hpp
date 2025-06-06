//
// Created by andy on 6/5/2025.
//

#pragma once

#include "game/render/render_device.hpp"

namespace game {

    class Shader {
      public:
        Shader(const std::shared_ptr<RenderDevice>& renderDevice, const std::vector<vk::ShaderCreateInfoEXT> &createInfos);

        [[nodiscard]] inline bool isLinked() const noexcept { return m_IsLinked; }

        [[nodiscard]] inline const std::vector<vk::ShaderStageFlagBits> &stages() const noexcept { return m_Stages; }

        [[nodiscard]] inline vk::ShaderStageFlags stageFlags() const noexcept { return m_StageFlags; }

        [[nodiscard]] inline vk::ShaderStageFlags allowedNextStageFlags() const { return m_AllowedNextStageFlags; }

        void bind(const vk::raii::CommandBuffer &cmd) const;

      private:
        vk::raii::ShaderEXTs       m_Shaders;
        std::vector<vk::ShaderEXT> m_ShadersI; // it's stupid but the command buffer won't accept an array of the raii shaders so I have to use a separate vector for these
        bool                       m_IsLinked;

        // stages can be used to determine the stage of each shader object
        // stageFlags is best for validating shader bindings (make sure that we don't make a material with 2 fragment shaders since only one will be used)
        std::vector<vk::ShaderStageFlagBits> m_Stages;
        vk::ShaderStageFlags                 m_StageFlags;

        vk::ShaderStageFlags m_AllowedNextStageFlags;
    };

} // namespace game

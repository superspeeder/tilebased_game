//
// Created by andy on 6/5/2025.
//

#include "shader_object.hpp"
#include "game/asset/common_parse.hpp"

#include <iostream>
#include <nlohmann/json.hpp>

namespace game {
    Shader::Shader(const std::shared_ptr<RenderDevice> &renderDevice, const std::vector<vk::ShaderCreateInfoEXT> &createInfos, const asset_id_t assetId, const std::string &name)
        : Asset(assetId, name), m_Shaders(renderDevice->device(), createInfos) {
        m_Stages.reserve(createInfos.size());
        m_ShadersI.reserve(createInfos.size());
        bool        anyNotLinked = false;
        bool        anyLinked    = false;
        std::size_t i            = 0;

        for (const auto &createInfo : createInfos) {
            if (createInfo.flags & vk::ShaderCreateFlagBitsEXT::eLinkStage) {
                anyLinked = true;
            } else {
                anyNotLinked = true;
            }

            m_Stages.push_back(createInfo.stage);
            m_StageFlags |= createInfo.stage;
            m_ShadersI.push_back(m_Shaders[i++]);
        }

        if (anyLinked && anyNotLinked)
            throw std::invalid_argument("Mixed linked and unlinked shaders in single asset");

        m_IsLinked = anyLinked;
    }

    void Shader::bind(const vk::raii::CommandBuffer &cmd) const {
        cmd.bindShadersEXT(m_Stages, m_ShadersI);
    }

    static vk::ShaderStageFlags inferAllowedNext(const vk::ShaderStageFlagBits currentStage, vk::ShaderStageFlags stagesInLinkedShader = vk::ShaderStageFlags{0U}) {
        if (stagesInLinkedShader == vk::ShaderStageFlags{0U}) {
            stagesInLinkedShader = currentStage; // not linked
        }

        switch (currentStage) {
        case vk::ShaderStageFlagBits::eVertex:
            if (stagesInLinkedShader & vk::ShaderStageFlagBits::eTessellationControl) {
                return vk::ShaderStageFlagBits::eTessellationControl;
            }

            if (stagesInLinkedShader & vk::ShaderStageFlagBits::eTessellationEvaluation) {
                return vk::ShaderStageFlagBits::eTessellationControl;
            }

            if (stagesInLinkedShader & vk::ShaderStageFlagBits::eGeometry) {
                return vk::ShaderStageFlagBits::eTessellationControl | vk::ShaderStageFlagBits::eGeometry;
            }

            return vk::ShaderStageFlagBits::eTessellationControl | vk::ShaderStageFlagBits::eGeometry | vk::ShaderStageFlagBits::eFragment;

        case vk::ShaderStageFlagBits::eTessellationControl:
            if (stagesInLinkedShader & vk::ShaderStageFlagBits::eTessellationEvaluation) {
                return vk::ShaderStageFlagBits::eTessellationEvaluation;
            }

            if (stagesInLinkedShader & vk::ShaderStageFlagBits::eGeometry) {
                return vk::ShaderStageFlagBits::eTessellationEvaluation | vk::ShaderStageFlagBits::eGeometry;
            }

            return vk::ShaderStageFlagBits::eTessellationEvaluation | vk::ShaderStageFlagBits::eGeometry | vk::ShaderStageFlagBits::eFragment;

        case vk::ShaderStageFlagBits::eTessellationEvaluation:
            if (stagesInLinkedShader & vk::ShaderStageFlagBits::eGeometry) {
                return vk::ShaderStageFlagBits::eGeometry;
            }

            return vk::ShaderStageFlagBits::eGeometry | vk::ShaderStageFlagBits::eFragment;

        case vk::ShaderStageFlagBits::eGeometry:
            return vk::ShaderStageFlagBits::eFragment;

        case vk::ShaderStageFlagBits::eFragment:
        case vk::ShaderStageFlagBits::eCompute:
            return vk::ShaderStageFlags{0U};

        default:
            throw std::invalid_argument("Invalid shader stage (cannot infer)");
        }
    }

    Shader *UnlinkedShaderAssetLoader::load(
        const Entry &entry, [[maybe_unused]] const std::nullptr_t &, asset_id_t id, const std::string &name, const AssetLoaderContext &loaderContext
    ) const {
        const auto &[metadata, data] = entry;
        if (metadata.shaderOptions.flags & vk::ShaderCreateFlagBitsEXT::eLinkStage)
            throw std::invalid_argument("Cannot set link-stage flag on unlinked shader object");

        vk::ShaderCreateInfoEXT createInfo{};
        createInfo.codeSize = data.size();
        createInfo.pCode    = reinterpret_cast<const uint32_t *>(data.data());
        createInfo.flags    = metadata.shaderOptions.flags;
        createInfo.pName    = metadata.shaderOptions.entryPoint.c_str();
        createInfo.codeType = vk::ShaderCodeTypeEXT::eSpirv;

        createInfo.stage = metadata.shaderOptions.stage;

        if (metadata.shaderOptions.allowedNextStages.has_value()) {
            createInfo.nextStage = metadata.shaderOptions.allowedNextStages.value();
        } else {
            createInfo.nextStage = inferAllowedNext(metadata.shaderOptions.stage);
        }

        createInfo.setSetLayouts(metadata.genericShaderOptions.descriptorSetLayouts);
        createInfo.setPushConstantRanges(metadata.genericShaderOptions.pushConstantRanges);

        return new Shader(loaderContext.renderSystem->renderDevice(), {createInfo}, id, name);
    }

    UnlinkedShaderManifest UnlinkedShaderAssetLoader::loadManifest(
        const std::size_t size, const unsigned char *data, const std::nullptr_t &options, const asset_id_t id, const std::string &name, const AssetLoaderContext &loaderContext
    ) const {
        nlohmann::json json = nlohmann::json::parse(data, data + size);
        if (!json.is_object())
            throw std::invalid_argument("Failed to parse unlinked shader '" + name + "': Metadata root is not a json object");
        const auto file  = json["file"].get<std::string>();
        const auto stage = parseStageBit(json["stage"].get<std::string>());
        const auto entry = json["entry"];

        std::optional<vk::ShaderStageFlags> allowedNextFlags = std::nullopt;
        if (json.contains("next")) {
            const auto next  = json["next"];
            allowedNextFlags = vk::ShaderStageFlags{0U};

            if (!next.is_array())
                throw std::invalid_argument("Failed to parse unlinked shader '" + name + "': List of allowed next stages is not an array");

            for (const auto &nextStage : next) {
                allowedNextFlags.value() |= parseStageBit(nextStage.get<std::string>());
            }
        }

        std::vector<vk::PushConstantRange> pcrs;
        for (const auto &pcrArr = json["push_constant_ranges"]; const auto &pcr : pcrArr) {
            pcrs.push_back(parsePcrJson(pcr));
        }

        // TODO: descriptor set layouts.
        //       descriptor set layouts is waiting on the asset manager + asset resolver to be set up
        return UnlinkedShaderManifest(
            UnlinkedShaderObjectOptions{PerShaderObjectOptions{stage, allowedNextFlags, entry, vk::ShaderCreateFlagsEXT{0U}}, GenericShaderObjectOptions{{}, pcrs}}, file
        );
    }

    Shader *LinkedShaderAssetLoader::load(
        const std::vector<Entry> &entries, const LinkedShaderManifest *manifest, [[maybe_unused]] const std::nullptr_t &, const asset_id_t id, const std::string &name,
        const AssetLoaderContext &loaderContext
    ) const {
        std::vector<vk::ShaderCreateInfoEXT> shaderCreateInfos;
        shaderCreateInfos.reserve(entries.size());
        vk::ShaderStageFlags stagesInLink{};
        for (const auto &[metadata, data] : entries)
            stagesInLink |= metadata.stage;
        for (const Entry &entry : entries) {
            shaderCreateInfos.push_back(_makeCreateInfo(entry, manifest->genericOptions, stagesInLink));
        }

        return new Shader(loaderContext.renderSystem->renderDevice(), shaderCreateInfos, id, name);
    }

    LinkedShaderManifest LinkedShaderAssetLoader::loadManifest(
        const std::size_t size, const unsigned char *data, [[maybe_unused]] const std::nullptr_t &, asset_id_t id, const std::string &name, const AssetLoaderContext &loaderContext
    ) const {
        LinkedShaderManifest manifest{};

        nlohmann::json json = nlohmann::json::parse(data, data + size);
        if (!json.is_object())
            throw std::invalid_argument("Invalid linked shader manifest: json root must be an object");

        const auto stagesArr = json["stages"];
        if (!stagesArr.is_array())
            throw std::invalid_argument("Invalid linked shader manifest: stages must be an array");

        for (const auto &stage : stagesArr) {
            if (!stage.is_object())
                throw std::invalid_argument("Invalid linked shader manifest: root array must contain objects only");

            const std::string             filename   = stage["file"].get<std::string>();
            const vk::ShaderStageFlagBits stageBit   = parseStageBit(stage["stage"].get<std::string>());
            const std::string             entryPoint = stage["entry"].get<std::string>();

            std::optional<vk::ShaderStageFlags> allowedNextFlags = std::nullopt;
            if (stage.contains("next")) {
                const auto next  = stage["next"];
                allowedNextFlags = vk::ShaderStageFlags{0U};

                if (!next.is_array())
                    throw std::invalid_argument("Invalid linked shader manifest: stage next stages must be an array");
                for (const auto &nextStage : next) {
                    allowedNextFlags.value() |= parseStageBit(nextStage.get<std::string>());
                }
            }

            manifest.entries.push_back(
                LinkedShaderManifest::Entry{
                    .data =
                        PerShaderObjectOptions{
                            .stage = stageBit, .allowedNextStages = allowedNextFlags, .entryPoint = entryPoint, .flags = vk::ShaderCreateFlagBitsEXT::eLinkStage
                        },
                    .path = filename,
                }
            );
        }

        std::vector<vk::PushConstantRange> pcrs;
        for (const auto &pcrArr = json["push_constant_ranges"]; const auto &pcr : pcrArr) {
            pcrs.push_back(parsePcrJson(pcr));
        }

        // TODO: descriptor set layouts.
        //       descriptor set layouts is waiting on the asset manager + asset resolver to be set up
        manifest.genericOptions = {{}, pcrs};

        return manifest;
    }

    vk::ShaderCreateInfoEXT LinkedShaderAssetLoader::_makeCreateInfo(const Entry &entry, const GenericShaderObjectOptions &options, const vk::ShaderStageFlags stagesInLink) {
        vk::ShaderCreateInfoEXT createInfo{};
        createInfo.flags    = entry.metadata.flags | vk::ShaderCreateFlagBitsEXT::eLinkStage;
        createInfo.codeSize = entry.data.size();
        createInfo.pCode    = reinterpret_cast<const uint32_t *>(entry.data.data());
        createInfo.pName    = entry.metadata.entryPoint.c_str();
        createInfo.codeType = vk::ShaderCodeTypeEXT::eSpirv;
        createInfo.stage    = entry.metadata.stage;
        if (entry.metadata.allowedNextStages.has_value()) {
            createInfo.nextStage = entry.metadata.allowedNextStages.value();
        } else {
            createInfo.nextStage = inferAllowedNext(entry.metadata.stage, stagesInLink);
        }

        createInfo.setSetLayouts(options.descriptorSetLayouts);
        createInfo.setPushConstantRanges(options.pushConstantRanges);

        return createInfo;
    }
} // namespace game

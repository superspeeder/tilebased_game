//
// Created by andy on 6/5/2025.
//

#pragma once

#include "game/asset/asset.hpp"
#include "game/asset/asset_loader.hpp"
#include "game/render/render_device.hpp"

namespace game {

    /**
     * @brief Shader Object asset
     * Represents a linked set of shader objects or an unlinked shader object
     */
    class Shader : public Asset<Shader> {
      public:
        /**
         * @brief Primary shader constructor. You should likely use one of the shader asset loaders instead of this
         * @param renderDevice The render device to construct this shader using
         * @param createInfos A list of shader object create infos to use
         * @param assetId The id of the shader asset (defaults to 0)
         * @param name The name of the shader asset (defaults to "")
         */
        Shader(const std::shared_ptr<RenderDevice> &renderDevice, const std::vector<vk::ShaderCreateInfoEXT> &createInfos, asset_id_t assetId = 0, const std::string &name = "");

        /**
         * @return If the shader object this shader contains are linked
         */
        [[nodiscard]] inline bool isLinked() const noexcept { return m_IsLinked; }

        /**
         * @return A list of stage flag bits (in the order the shader objects are stored internally).
         */
        [[nodiscard]] inline const std::vector<vk::ShaderStageFlagBits> &stages() const noexcept { return m_Stages; }

        /**
         * @return The flags representing which stages are bound to this shader.
         */
        [[nodiscard]] inline vk::ShaderStageFlags stageFlags() const noexcept { return m_StageFlags; }

        /**
         * @brief Bind the shaders this asset for use.
         * @param cmd The command buffer to bind this on
         */
        void bind(const vk::raii::CommandBuffer &cmd) const;

        [[nodiscard]] inline const vk::ShaderEXT &operator*() const { return *m_Shaders[0]; };

      private:
        vk::raii::ShaderEXTs       m_Shaders;
        std::vector<vk::ShaderEXT> m_ShadersI; // it's stupid but the command buffer won't accept an array of the raii shaders so I have to use a separate vector for these
        bool                       m_IsLinked;

        // stages can be used to determine the stage of each shader object
        // stageFlags is best for validating shader bindings (make sure that we don't make a material with 2 fragment shaders since only one will be used)
        std::vector<vk::ShaderStageFlagBits> m_Stages;
        vk::ShaderStageFlags                 m_StageFlags{0U};
    };

    /**
     * @brief The metadata associated with a single shader file.
     */
    struct PerShaderObjectOptions {
        vk::ShaderStageFlagBits             stage;
        std::optional<vk::ShaderStageFlags> allowedNextStages;
        std::string                         entryPoint = "main";
        vk::ShaderCreateFlagsEXT            flags      = vk::ShaderCreateFlagsEXT{0U};
    };

    /**
     * @brief The generic metadata associated with a shader object or collection of shader objects.
     */
    struct GenericShaderObjectOptions {
        std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
        std::vector<vk::PushConstantRange>   pushConstantRanges;
        // TODO: specialization info
    };

    /**
     * @brief The options of an unlinked shader.
     */
    struct UnlinkedShaderObjectOptions {
        PerShaderObjectOptions     shaderOptions;
        GenericShaderObjectOptions genericShaderOptions;
    };

    class UnlinkedShaderManifest final : public AssetPlusMetadataManifest<UnlinkedShaderObjectOptions> {
      public:
        UnlinkedShaderObjectOptions options;
        std::filesystem::path       path;

        inline UnlinkedShaderManifest(const UnlinkedShaderObjectOptions &options, const std::filesystem::path &path) : options(options), path(path) {}

        inline Entry fileToLoad() const override { return Entry{options, path}; }
    };

    /**
     * @brief Asset loader for unlinked shader objects.
     */
    class UnlinkedShaderAssetLoader final : public AssetPlusMetadataAssetLoader<Shader, std::nullptr_t, UnlinkedShaderObjectOptions, UnlinkedShaderManifest> {
      public:
        Shader *load(const Entry &entry, const std::nullptr_t &options, asset_id_t id, const std::string &name, const AssetLoaderContext &loaderContext) const override;

        UnlinkedShaderManifest loadManifest(
            std::size_t size, const unsigned char *data, const std::nullptr_t &options, asset_id_t id, const std::string &name, const AssetLoaderContext &loaderContext
        ) const override;
    };

    /**
     * @brief Manifest for linked shader object assets.
     */
    struct LinkedShaderManifest final : public MultiFileAssetManifest<PerShaderObjectOptions> {
        std::vector<Entry> entries;
        GenericShaderObjectOptions genericOptions;

        [[nodiscard]] inline std::vector<Entry> filesToLoad() const override { return entries; }

        ~LinkedShaderManifest() override = default;
    };

    /**
     * @brief Asset loader for linked shader objects
     */
    class LinkedShaderAssetLoader final : public MultiFileAssetLoader<Shader, std::nullptr_t, PerShaderObjectOptions, LinkedShaderManifest> {
      public:
        [[nodiscard]] Shader *load(
            const std::vector<Entry> &entries, const LinkedShaderManifest *manifest, const std::nullptr_t &options, asset_id_t id, const std::string &name,
            const AssetLoaderContext &loaderContext
        ) const override;

        LinkedShaderManifest loadManifest(
            std::size_t size, const unsigned char *data, const std::nullptr_t &options, asset_id_t id, const std::string &name, const AssetLoaderContext &loaderContext
        ) const override;

      private:
        /**
         * @brief Utility for generating createinfo structs from loaded data.
         * @param entry The asset entry containing data and metadata
         * @param options The loading options
         * @param stagesInLink The stages present in the linked shader
         * @return A filled out createinfo struct.
         */
        static vk::ShaderCreateInfoEXT _makeCreateInfo(const Entry &entry, const GenericShaderObjectOptions &options, vk::ShaderStageFlags stagesInLink);
    };


} // namespace game

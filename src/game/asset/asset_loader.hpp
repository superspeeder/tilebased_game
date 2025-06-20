#pragma once

#include "game/asset/asset.hpp"
#include "game/render/render_system.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <streambuf>

#include <typeindex>

namespace game {
    class AssetManager;

    std::filesystem::path assetDir();

    inline std::filesystem::path assetPath(const std::filesystem::path& path) {
        return assetDir() / path;
    };

    /**
     * @brief Context data passed to every asset load.
     */
    struct AssetLoaderContext {
        /**
         * @brief The render system object. Important for many kind of assets to be loaded correctly.
         */
        std::shared_ptr<RenderSystem> renderSystem;
        AssetManager                 *assetManager;
    };

    /**
     * @brief Namespace containing asset utility functions which aren't meant for non-asset use
     */
    namespace asset_util {
        static inline std::ifstream openFile(const std::filesystem::path &path) {
            const auto fullPath = assetPath(path);
            if (!std::filesystem::exists(fullPath)) throw std::invalid_argument("File '" + fullPath.string() + "' does not exist");
            std::ifstream file(fullPath, std::ios::in | std::ios::binary | std::ios::ate);
            if (file.rdstate() != std::ios_base::goodbit) {
                throw std::invalid_argument("Couldn't open file: " + path.string());
            }
            return file;
            
        }

        /**
         * @brief Read a file for asset loading.
         * @param path The path to the asset file (relative to the assets directory)
         * @return The binary contents of that asset file.
         */
        static inline std::vector<unsigned char> readFile(const std::filesystem::path &path) {
            auto file = openFile(path);
            const std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);
            std::vector<unsigned char> buffer(size);
            file.read(reinterpret_cast<char *>(buffer.data()), size);
            file.close();

            return buffer;
        }

    } // namespace asset_util

    template <typename O>
    class GenericAssetLoader {
      public:
        virtual ~GenericAssetLoader() = default;
        virtual AssetBase *genericLoad(
            std::size_t size, const unsigned char *data, const O &options, asset_id_t id, const std::string &name, const AssetLoaderContext &loaderContext
        ) const = 0;

        virtual AssetBase *genericLoadAssetFromFile(const std::filesystem::path &path, const O &options, const asset_id_t id, const AssetLoaderContext &loaderContext) const {
            const auto buffer = asset_util::readFile(path);
            return genericLoad(buffer.size(), buffer.data(), options, id, path.string(), loaderContext);
        }
    };

    /**
     * @brief Base class for asset loaders
     * @tparam T The loaded asset type
     * @tparam O The load-time options type
     */
    template <asset_type T, typename O>
    class AssetLoader : public GenericAssetLoader<O> {
      public:
        using asset_t   = T;
        using options_t = O;

        AssetLoader()           = default;
        ~AssetLoader() override = default;

        /** @brief Load an asset from a chunk of data and some options
         *  The asset metadata database should provide information on options.
         *
         *  This doesn't require the input to be a vector since when loading from asset packs, the data won't be seperated from the rest of the pack (instead you are just
         * responsible for not overrunning the asset).
         *
         *  @param size the size of the input data
         *  @param data a pointer to the start of the input data
         *  @param options The options for the asset being loaded (for example, a shader being loaded as glsl, hlsl, or spir-v)
         *  @param id The id of the loaded asset
         *  @param name The name of the asset. Used for asset reference resolution and easy lookups (not as efficient as an id based lookup).
         *  @param loaderContext The asset loader context to use when loading this asset. Contains information about source (NYI) and gives access to various systems which are
         * needed to actually load the asset (for example, the render system so that data can be pushed to the GPU).
         *  @return A raw pointer to the loaded asset. This should be allocated with `new` and ownership is taken by the caller of this function.
         */
        virtual T *load(std::size_t size, const unsigned char *data, const O &options, asset_id_t id, const std::string &name, const AssetLoaderContext &loaderContext) const = 0;

        inline AssetBase *genericLoad(
            std::size_t size, const unsigned char *data, const O &options, asset_id_t id, const std::string &name, const AssetLoaderContext &loaderContext
        ) const override {
            return load(size, data, options, id, name, loaderContext);
        }

        /**
         * @brief Load an asset from a file.
         * @param path The path to the file to load this asset from (relative to the assets directory)
         * @param options The load options for this asset
         * @param id The id of the loaded asset
         * @param loaderContext The context for asset loading
         * @return A raw pointer to the loaded asset.
         * @see AssetLoader::load
         */
        virtual T *loadAssetFromFile(const std::filesystem::path &path, const O &options, const asset_id_t id, const AssetLoaderContext &loaderContext) {
            const auto buffer = asset_util::readFile(path);
            return load(buffer.size(), buffer.data(), options, id, path.string(), loaderContext);
        }

        /**
         * @brief Utility for loading assets from a file which only works when the options type is std::nullptr_t (which lets us just pass nullptr as the options)
         * @param path The path to the file to load this asset from (relative to the assets directory)
         * @param id The id of this asset
         * @param loaderContext The context for asset loading
         * @return A raw pointer to the loaded asset.
         */
        T *loadAssetFromFile(const std::filesystem::path &path, const asset_id_t id, const AssetLoaderContext &loaderContext)
            requires(std::same_as<O, std::nullptr_t>)
        {
            return loadAssetFromFile(path, nullptr, id, loaderContext);
        }

        // TODO: loadAssetFromCompressed and loadAssetFromPack
    };

    /**
     * Types fitting this concept are valid for multi-file asset data.
     */
    template <typename T>
    concept multifile_asset_data = std::copyable<T>;

    /**
     * @brief A manifest for a multi-file asset. Responsible for identifying which files should be loaded.
     * @tparam D The per-entry metadata type
     */
    template <multifile_asset_data D>
    class MultiFileAssetManifest {
      public:
        /**
         * @brief A file entry in this manifest (contains a path relative to the asset directory and metadata).
         */
        struct Entry {
            D                     data;
            std::filesystem::path path;
        };

        virtual ~MultiFileAssetManifest() = default;

        /**
         * @brief Get a list of files to load for the asset this manifest represents.
         * @return A list of path+metadata pairs which is used to actually load the asset
         */
        virtual std::vector<Entry> filesToLoad() const = 0;
    };

    /**
     * @brief
     * @tparam T The loaded asset type
     * @tparam O The asset options type
     * @tparam D The per-file asset metadata type
     * @tparam M The asset manifest type
     */
    template <asset_type T, typename O, multifile_asset_data D, std::derived_from<MultiFileAssetManifest<D>> M>
    class MultiFileAssetLoader : public AssetLoader<T, O> {
      public:
        /**
         * @brief A file entry for asset loading containing metadata extracted from the manifest and the binary contents of the file.
         */
        struct Entry {
            D                          metadata;
            std::vector<unsigned char> data;
        };

        T *load(
            const std::size_t size, const unsigned char *data, const O &options, const asset_id_t id, const std::string &name, const AssetLoaderContext &loaderContext
        ) const final {
            const M manifest = loadManifest(size, data, options, id, name, loaderContext);

            const auto         fileEntries = manifest.filesToLoad();
            std::vector<Entry> entries;
            entries.reserve(fileEntries.size());
            for (const auto &fileEntry : fileEntries) {
                entries.push_back(Entry{fileEntry.data, asset_util::readFile(fileEntry.path)});
            }

            return load(entries, &manifest, options, id, name, loaderContext);
        };

        /**
         * @brief Load an asset from a list of entries, a manifest, and some options. This should likely be called internally only.
         * @param entries A list of file entries for this asset
         * @param manifest The manifest loaded for this multi-file asset
         * @param options The options for loading this asset
         * @param id The id of this asset
         * @param name The name of this asset
         * @param loaderContext The context to use for loading this asset (get things like asset resolver or render system from this)
         * @return A raw pointer to the loaded asset
         */
        virtual T *load(
            const std::vector<Entry> &entries, const M *manifest, const O &options, asset_id_t id, const std::string &name, const AssetLoaderContext &loaderContext
        ) const = 0;

        /**
         * @brief Load the manifest for an asset
         * @param size The size of the raw data
         * @param data The raw binary data
         * @param options The options for loading
         * @param id The id of this asset
         * @param name The name of this asset
         * @param loaderContext The context to use to load this asset
         * @return The manifest object that is loaded.
         */
        virtual M loadManifest(
            std::size_t size, const unsigned char *data, const O &options, asset_id_t id, const std::string &name, const AssetLoaderContext &loaderContext
        ) const = 0;
    };

    /**
     * @brief Metadata portion of an asset which contains a single data file and a metadata file.
     * @tparam D The type of the metadata.
     */
    template <multifile_asset_data D>
    class AssetPlusMetadataManifest : public MultiFileAssetManifest<D> {
      public:
        virtual typename MultiFileAssetManifest<D>::Entry fileToLoad() const = 0;

        std::vector<typename MultiFileAssetManifest<D>::Entry> filesToLoad() const final { return {fileToLoad()}; };
    };

    /**
     * @brief A simplified form of the multi-file asset loader designed for assets which are a single asset file and a single metadata file.
     * @tparam T The loaded asset type
     * @tparam O The options type
     * @tparam D The asset metadata type
     * @tparam M The manifest type
     */
    template <asset_type T, typename O, multifile_asset_data D, std::derived_from<AssetPlusMetadataManifest<D>> M>
    class AssetPlusMetadataAssetLoader : public MultiFileAssetLoader<T, O, D, M> {
      public:
        T *load(
            const std::vector<typename MultiFileAssetLoader<T, O, D, M>::Entry> &entries, [[maybe_unused]] const M *manifest, const O &options, const asset_id_t id,
            const std::string &name, const AssetLoaderContext &loaderContext
        ) const final {
            return load(entries.front(), options, id, name, loaderContext);
        };

        /**
         * @brief Load the asset from the metadata+binary pair and some options.
         * @param entry The metadata+binary of the asset being loaded
         * @param options The options for loading
         * @param id The id of the asset
         * @param name The name of the asset
         * @param loaderContext The loading context of the asset
         * @return A raw pointer to the asset
         *
         * @note Unlike most asset load functions under a multi-file asset loader, this does not accept the manifest since the asset metadata should exist only in the metadata time
         * (as there is only one data file) and there is no need for top-level metadata separate from that.
         */
        virtual T *load(
            const typename MultiFileAssetLoader<T, O, D, M>::Entry &entry, const O &options, asset_id_t id, const std::string &name, const AssetLoaderContext &loaderContext
        ) const = 0;
    };

    /**
     * @brief A simple asset loader for single json file assets
     * @tparam T The type of the loaded asset
     * @tparam O The load-time user provided options type
     */
    template <asset_type T, typename O>
    class JsonAssetLoader : public AssetLoader<T, O> {
      public:
        /**
         * @brief Override this for asset loading logic
         *
         * This is called by this classes implementation of the generic load function.
         */
        virtual T *load(const nlohmann::json &json, const O &options, asset_id_t id, const std::string &name, const AssetLoaderContext &loaderContext) const = 0;

        T *load(
            const std::size_t size, const unsigned char *data, const O &options, const asset_id_t id, const std::string &name, const AssetLoaderContext &loaderContext
        ) const final {
            const auto json = nlohmann::json::parse(data, data + size);
            return load(json, options, id, name, loaderContext);
        }
    };

    /**
     * @brief Generic data that is grabbed from asset metadata files
     */
    struct GenericAssetData {
        /**
         * @brief The name of the asset type (used to grab the loader). Required
         */
        std::string assetType;

        /**
         * @brief The name of the asset (if not present, the path relative to the asset directory will be used).
         */
        std::string assetName;

        /**
         * @brief For assets which are a source file + metadata, this lets you override the default source file.
         *
         * If not specified, the metadata file is source_filename.json
         */
        std::optional<std::string> fileOverride; // only used by assets which are AssetPlusMetadata types
        
        /**
         * @brief Static id values set by assets.
         *
         * If not required, the standard asset id provider for the loading context will be used.
         */
        std::optional<asset_id_t> staticId;
    };

    /**
     * Load an asset from a file in a generic way.
     *
     * This will read the metadata and use it to finish loading the asset.
     *
     * All assets are assumed to take the metadata file as the load parameter (likely this will trigger json being parsed twice. In the future that should be fixed, probably by restructuring the asset system to fit the new standards I'm setting about metadata).
     */
    AssetBase* loadAssetFromFileGeneric(const std::string& path, const AssetLoaderContext& loaderContext);

} // namespace game


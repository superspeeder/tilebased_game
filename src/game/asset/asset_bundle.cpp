//
// Created by andy on 6/17/2025.
//

#include "asset_bundle.hpp"

#include "game/asset/asset_manager.hpp"

namespace game {
    AssetBundle::AssetBundle(std::vector<GenericAssetRef> refs, const asset_id_t assetId, const std::string &name) : Asset(assetId, name), m_Refs(std::move(refs)) {}

    AssetBundle *AssetBundleLoader::load(
        const nlohmann::json &json, [[maybe_unused]] const std::nullptr_t &options, const asset_id_t id, const std::string &name, const AssetLoaderContext &loaderContext
    ) const {
        std::vector<GenericAssetRef> refs;
        if (!json.is_object())
            throw std::invalid_argument("Failed to parse asset bundle json: root must be an object");
        for (const auto &entry : json.items()) {
            if (!entry.value().is_array()) {
                throw std::invalid_argument("Failed to parse asset bundle json: each set of assets must be an array.");
            }

            const auto loader = loaderContext.assetManager->getLoader(entry.key());
            for (const auto &assetPath : entry.value()) {
                if (!assetPath.is_string()) {
                    throw std::invalid_argument(
                        "Failed to parse asset bundle json: each entry in an asset list must be a string containing the path to the asset (relative to the assets folder)."
                    );
                }

                refs.push_back(std::move(loaderContext.assetManager->loadFromFileUsing(loader, assetPath.get<std::string>())));
            }
        }

        return new AssetBundle(std::move(refs), id, name);
    }
} // namespace game

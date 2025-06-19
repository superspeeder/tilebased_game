//
// Created by andy on 6/17/2025.
//

#pragma once
#include "game/asset/asset.hpp"
#include "game/asset/asset_loader.hpp"

#include <vector>

namespace game {

    class AssetBundle final : public Asset<AssetBundle> {
    public:
        explicit AssetBundle(std::vector<GenericAssetRef> refs, asset_id_t assetId = 0, const std::string &name = "");

    private:
        std::vector<GenericAssetRef> m_Refs;

    };

    class AssetBundleLoader final : public JsonAssetLoader<AssetBundle, std::nullptr_t> {
      public:
        AssetBundle *load(
            const nlohmann::json &json, const std::nullptr_t &options, asset_id_t id, const std::string &name, const AssetLoaderContext &loaderContext
        ) const override;
    };

} // namespace game

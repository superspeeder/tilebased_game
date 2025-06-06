#pragma once

#include "game/asset/asset.hpp"
#include <streambuf>

namespace game {
    template<asset_type T, typename O>
    class AssetLoader {
    public:
        AssetLoader();
        ~AssetLoader();

        /** @brief Load an asset from a chunk of data and some options
         *  The asset metadata database should provide information on options.
         *
         *  @param input The stream of input data to load the asset from
         *  @param options The options for the asset being loaded (for example, a shader being loaded as glsl, hlsl, or spir-v)
         *  @param id The id of the loaded asset
         *  @return A raw pointer to the loaded asset. This should be allocated with `new` and ownership is taken by the caller of this function.
         */
        virtual T* load(std::basic_streambuf<unsigned char>& input, const O& options, asset_id_t id) const = 0;
    };
}

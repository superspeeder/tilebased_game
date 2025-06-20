#include "game/asset/asset_loader.hpp"
#include "game/asset/asset_manager.hpp"
#include <filesystem>
#include <optional>


#ifdef GAME_BUILD_DIST
#ifdef WIN32
#include <libloaderapi.h>
#endif

static bool executablePathLazy = false;
std::filesystem::path executablePath() {
    static std::filesystem::path self;
    if (!executablePathLazy) {
#if defined(WIN32)
        DWORD sz = GetModuleFileNameA(nullptr, nullptr, 0);
        char* buf = new char[sz];
        GetModuleFileNameA(nullptr, buf, sz);

#elif defined(__linux__)
        self = std::filesystem::canonical("/proc/self/exe");
#else
#error "Currently unsupported OS"
#endif
        executablePathLazy = true;
    }
    return self;
}
#endif

namespace game {
    static bool assetDirLazy = false;
    std::filesystem::path assetDir() {
        static std::filesystem::path path;
        if (!assetDirLazy) {
#ifdef GAME_BUILD_DIST
            path = executablePath().parent_path() / "assets";
#else
            path = std::filesystem::current_path() / "assets";
#endif
            assetDirLazy = true;
        }
        return path;
    }

    static AssetBase* loadAssetFromFileGenericInner(const std::string& path, const AssetLoaderContext& loaderContext) {
        auto file = asset_util::openFile(path);
        const auto& json = nlohmann::json::parse(file);
        file.close();

        const auto& type = json["type"].get<std::string>();
        
        std::string name = path;
        if (json.contains("name")) {
            name = json["name"].get<std::string>();
        }


        std::optional<std::string> fileOverride = std::nullopt;
        if (json.contains("file")) {
            fileOverride = json["file"].get<std::string>();
        }

        std::optional<asset_id_t> staticId = std::nullopt;
        if (json.contains("staticId")) {
            staticId = STATIC_ID_DOMAIN | json["staticId"].get<asset_id_t>();
        }


    }

    AssetBase* loadAssetFromFileGeneric(const std::string& path, const AssetLoaderContext& loaderContext) {
        // check for metadata file
        if (std::filesystem::exists(assetPath(path + ".json"))) {
            return loadAssetFromFileGenericInner(path + ".json", loaderContext);
        }

        return loadAssetFromFileGenericInner(path, loaderContext);
    }
}

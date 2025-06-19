//
// Created by andy on 6/14/2025.
//

#pragma once
#include "asset.hpp"
#include "asset_loader.hpp"
#include "game/utils.hpp"

#include <semaphore>
#include <thread>
#include <unordered_set>

namespace game {
    template <typename L>
    concept asset_loader = std::derived_from<L, AssetLoader<typename L::asset_t, typename L::options_t>>;

    template <typename L>
    concept default_constructible_asset_loader = asset_loader<L> && std::default_initializable<L>;

    // this is not a very smart loader (no threading support)
    class AssetManager {
      public:
        explicit AssetManager(const std::shared_ptr<RenderSystem> &renderSystem);
        ~AssetManager();

        AssetManager(const AssetManager &other)                = delete;
        AssetManager(AssetManager &&other) noexcept            = delete;
        AssetManager &operator=(const AssetManager &other)     = delete;
        AssetManager &operator=(AssetManager &&other) noexcept = delete;

        template <default_constructible_asset_loader T>
        AssetRef<typename T::asset_t> loadFromFile(const std::string &filename, const typename T::options_t &options) {
            if (const auto it = m_AssetsByName.find(filename); it != m_AssetsByName.end()) {
                return AssetRef<typename T::asset_t>(dynamic_cast<typename T::asset_t*>(it->second));
            }

            T          loader{};
            const auto rawAsset = loader.loadAssetFromFile(filename, options, generateId(), m_Context);
            return std::move(registerAsset(rawAsset));
        }

        template <default_constructible_asset_loader T>
            requires(std::same_as<typename T::options_t, std::nullptr_t>)
        AssetRef<typename T::asset_t> loadFromFile(const std::string &filename) {
            return std::move(loadFromFile<T>(filename, nullptr));
        }

        const GenericAssetLoader<std::nullptr_t> *getLoader(const std::string &loaderName) const;
        GenericAssetRef                           loadFromFileUsing(const std::string &loaderName, const std::string &filename);
        GenericAssetRef                           loadFromFileUsing(const GenericAssetLoader<std::nullptr_t> *loader, const std::string &filename);

        template <asset_type T>
        AssetRef<T> registerAsset(T *asset) {
            registerRawAsset(asset);
            return std::move(AssetRef<T>(asset));
        }

        GenericAssetRef registerAssetGeneric(AssetBase *asset);

        [[nodiscard]] bool hasAsset(asset_id_t id) const;
        [[nodiscard]] bool hasAsset(const std::string &name) const;
        [[nodiscard]] bool hasAsset(AssetBase *asset) const;

        template <asset_type T>
        [[nodiscard]] bool hasAsset(const AssetHandle<T> &asset) const {
            return hasAsset(asset.id);
        };

        void queueForRemoval(asset_id_t id);
        void queueForRemoval(const std::string &name);
        void queueForRemoval(AssetBase *asset);

        template <asset_type T>
        void queueForRemoval(const AssetHandle<T> &asset) {
            queueForRemoval(asset.id);
        };

        void startDeletionCycle();
        void beginDeletionThread();
        void deleteWaitingAssets();
        void endDeletionThread();
        void finalEndDeletionThread();
        void removeRecursive();

        void populateLoaders();

        template <typename T>
        AssetRef<T> get(const std::string &name) const {
            if (const auto it = m_AssetsByName.find(name); it != m_AssetsByName.end()) {
                return AssetRef<T>(dynamic_cast<T *>(it->second));
            }
            return AssetRef<T>();
        }

        template <typename T>
        AssetRef<T> get(const asset_id_t id) const {
            if (const auto it = m_Assets.find(id); it != m_Assets.end()) {
                return AssetRef<T>(dynamic_cast<T *>(it->second));
            }
            return AssetRef<T>();
        }

      protected:
        asset_id_t generateId();
        void       registerRawAsset(AssetBase *asset);

      private:
        std::unordered_map<asset_id_t, AssetBase *>  m_Assets;
        std::unordered_map<std::string, AssetBase *> m_AssetsByName;
        std::unordered_set<AssetBase *>              m_LoadedAssets;
        asset_counter_t                              m_AssetCounter = 1;

        std::unordered_map<std::string, GenericAssetLoader<std::nullptr_t> *> m_AssetLoaders;

        concurrent_queue<AssetBase *> m_RemovalQueue; // these assets are queued for removal next time the system

        std::mutex            m_LoadedSetMutex;
        std::jthread          m_AssetDeletionThread;
        std::binary_semaphore m_CycleSemaphore;
        std::atomic_flag      m_DeletionWaitingFlag;

        std::shared_ptr<RenderSystem> m_RenderSystem;
        AssetLoaderContext            m_Context{m_RenderSystem, this};

        void deletionThread(std::stop_token);
    };

} // namespace game

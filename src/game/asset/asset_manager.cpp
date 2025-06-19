//
// Created by andy on 6/14/2025.
//

#include "asset_manager.hpp"

#include "game/asset/asset_bundle.hpp"
#include "game/render/pipeline_layout.hpp"
#include "game/render/shader_object.hpp"
#include "spdlog/spdlog.h"

namespace game {
    AssetManager::AssetManager(const std::shared_ptr<RenderSystem> &renderSystem) : m_CycleSemaphore(0), m_RenderSystem(renderSystem) {
        populateLoaders();
    }

    AssetManager::~AssetManager() {
        m_LoadedSetMutex.lock();
        removeRecursive();
        for (const auto &asset : m_LoadedAssets) {
            spdlog::info("Destroy: {} [{}]", asset->name(), asset->_id());
            delete asset; // because this is getting destroyed we don't need to bother with any other data structure clearing.
        }
        m_LoadedSetMutex.unlock();

        for (const auto &val : m_AssetLoaders | std::views::values) {
            delete val;
        }
    }

    const GenericAssetLoader<std::nullptr_t> *AssetManager::getLoader(const std::string &loaderName) const {
        return m_AssetLoaders.find(loaderName)->second;
    }

    GenericAssetRef AssetManager::loadFromFileUsing(const std::string &loaderName, const std::string &filename) {
        return loadFromFileUsing(getLoader(loaderName), filename);
    }

    GenericAssetRef AssetManager::loadFromFileUsing(const GenericAssetLoader<std::nullptr_t> *loader, const std::string &filename) {
        if (const auto it = m_AssetsByName.find(filename); it != m_AssetsByName.end()) {
            return it->second;
        }

        const auto rawAsset = loader->genericLoadAssetFromFile(filename, nullptr, generateId(), m_Context);
        registerRawAsset(rawAsset);
        return std::move(GenericAssetRef(rawAsset));
    }

    GenericAssetRef AssetManager::registerAssetGeneric(AssetBase *asset) {
        registerRawAsset(asset);
        return asset;
    }

    bool AssetManager::hasAsset(const asset_id_t id) const {
        return m_Assets.contains(id);
    }

    bool AssetManager::hasAsset(const std::string &name) const {
        return m_AssetsByName.contains(name);
    }

    bool AssetManager::hasAsset(AssetBase *asset) const {
        return m_LoadedAssets.contains(asset);
    }

    void AssetManager::queueForRemoval(const asset_id_t id) {
        if (const auto &it = m_Assets.find(id); it != m_Assets.end()) {
            m_RemovalQueue.enqueue(it->second);
        }
    }

    void AssetManager::queueForRemoval(const std::string &name) {
        if (const auto &it = m_AssetsByName.find(name); it != m_AssetsByName.end()) {
            m_RemovalQueue.enqueue(it->second);
        }
    }

    void AssetManager::queueForRemoval(AssetBase *const asset) {
        m_RemovalQueue.enqueue(asset);
    }

    void AssetManager::startDeletionCycle() {
        m_CycleSemaphore.release(1);
    }

    void AssetManager::beginDeletionThread() {
        m_AssetDeletionThread = std::jthread([this](std::stop_token stopToken) { this->deletionThread(stopToken); });
    }

    void AssetManager::deleteWaitingAssets() {
        if (m_DeletionWaitingFlag.test(std::memory_order::relaxed)) {
            std::lock_guard lock(m_LoadedSetMutex); // this will be changing during this loop
            while (!m_RemovalQueue.empty()) {
                auto asset = m_RemovalQueue.dequeue();
                if (const auto &it = m_LoadedAssets.find(asset); it != m_LoadedAssets.end()) {
                    m_Assets.erase((*it)->m_Id);
                    m_AssetsByName.erase((*it)->m_Name);
                    m_LoadedAssets.erase(it);

                    spdlog::info("Destroy: {} [{}]", asset->name(), asset->_id());
                    delete asset;
                }
            }
            m_DeletionWaitingFlag.clear(std::memory_order::relaxed);
        }
    }

    void AssetManager::endDeletionThread() {
        m_AssetDeletionThread.request_stop();
        m_CycleSemaphore.release(1); // wake it up
    }

    void AssetManager::finalEndDeletionThread() {
        m_AssetDeletionThread.join();
    }

    void AssetManager::removeRecursive() {
        bool assetRemovedLastCycle;
        do {
            assetRemovedLastCycle = false;
            std::deque<AssetBase *> assetsToRemove;
            for (const auto &asset : m_LoadedAssets) {
                if (!asset->isKeepAlive() && asset->refcount() == 0) {
                    assetsToRemove.push_back(asset);
                }
            }

            while (!assetsToRemove.empty()) {
                assetRemovedLastCycle = true;
                auto asset            = assetsToRemove.front();
                assetsToRemove.pop_front();
                m_LoadedAssets.erase(asset);
                m_AssetsByName.erase(asset->m_Name);
                m_Assets.erase(asset->m_Id);
                spdlog::info("Destroy: {} [{}]", asset->name(), asset->_id());
                delete asset;
            }
        } while (assetRemovedLastCycle);
    }

    void AssetManager::populateLoaders() {
        m_AssetLoaders["linked_shader"]   = new LinkedShaderAssetLoader();
        m_AssetLoaders["unlinked_shader"] = new UnlinkedShaderAssetLoader();
        m_AssetLoaders["pipeline_layout"] = new PipelineLayoutLoader();
        m_AssetLoaders["asset_bundle"]    = new AssetBundleLoader();
    }

    asset_id_t AssetManager::generateId() {
        return m_AssetCounter.fetch_add(1, std::memory_order::relaxed); // the counter is relaxed since the actual number doesn't matter all that much, just that it's unique.
    }

    void AssetManager::registerRawAsset(AssetBase *asset) {
        {
            std::lock_guard lock(m_LoadedSetMutex);
            m_LoadedAssets.insert(asset);
        }

        m_Assets.insert({asset->_id(), asset});
        m_AssetsByName.insert({asset->name(), asset});
    }

    // this function is the thread which queues assets to be deleted when they become unreferenced (asset gc)
    void AssetManager::deletionThread(std::stop_token stopToken) {
        while (!stopToken.stop_requested()) {
            m_CycleSemaphore.acquire();
            if (stopToken.stop_requested())
                break;

            {
                bool            mark = false;
                std::lock_guard lock(m_LoadedSetMutex);
                for (const auto &asset : m_LoadedAssets) {
                    if (!asset->isKeepAlive() && asset->refcount() == 0) {
                        m_RemovalQueue.enqueue(asset);
                        mark = true;
                    }
                }

                if (mark) {
                    m_DeletionWaitingFlag.test_and_set(std::memory_order_relaxed);
                }
            }
        }
    }
} // namespace game

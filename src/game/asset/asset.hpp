//
// Created by andy on 6/5/2025.
//

#pragma once

#include <atomic>
#include <concepts>

namespace game {
#ifdef ASSETS_64BIT_ID
    using asset_id_t      = uint64_t;
    using asset_counter_t = std::atomic_uint64_t;
#else
    using asset_id_t      = uint32_t;
    using asset_counter_t = std::atomic_uint32_t;
#endif

#ifndef ASSETS_MAX_REFERENCES
#define ASSETS_MAX_REFERENCES 4294967295U
#endif

#if ASSETS_MAX_REFERENCES <= 255U
    using asset_rc_t = std::atomic_uint8_t;
#elif ASSETS_MAX_REFERENCES <= 65535U
    using asset_rc_t = std::atomic_uint16_t;
#elif ASSETS_MAX_REFERENCES <= 4294967295U
    using asset_rc_t = std::atomic_uint32_t;
#else
    using asset_rc_t = std::atomic_uint64_t;
#endif

#ifdef ASSETS_REFCOUNT_BOUNDS_CHECKS
#if ASSETS_MAX_REFERENCES == 18446744073709551615U || ASSETS_MAX_REFERENCES == 4294967295U || ASSETS_MAX_REFERENCES == 65535U || ASSETS_MAX_REFERENCES == 255U
    // max references is equal to the max of an unsigned type, so passing increment bounds **will** overflow
#define ASSET_OVERFLOW_WARNING " (results in integer overflow, expect broken state)"
#else
#define ASSET_OVERFLOW_WARNING
#endif

#include <cassert>
#define asset_rc_assert_inc(s) assert(((s) < (ASSETS_MAX_REFERENCES)) && ("Cannot increment reference counter: Too many references to asset" ASSET_OVERFLOW_WARNING))
#define asset_rc_assert_dec(s) assert(((s) > 0) && ("Cannot decrement reference counter: No existing references to asset (will cause integer underflow, expect broken state)"))
#else
#define asset_rc_assert_inc(s) (s)
#define asset_rc_assert_dec(s) (s)
#endif

    template <typename T>
    class AssetReference;

    template <typename T>
    class AssetWeak;

    class AssetBase {
      public:
        inline explicit AssetBase(const asset_id_t id) : m_Id(id) {}

      protected:
        [[nodiscard]] inline asset_id_t _id() const noexcept { return m_Id; };

        inline void incRef() noexcept { asset_rc_assert_inc(m_RefCount++); }

        inline void decRef() noexcept { asset_rc_assert_dec(m_RefCount--); }

      private:
        asset_id_t m_Id;
        asset_rc_t m_RefCount;
    };

    template <typename T>
    struct AssetHandle;

    template <typename T>
    class Asset : public AssetBase {
      public:
        using handle_t = AssetHandle<T>;
        using ref_t    = AssetReference<T>;
        using weak_t   = AssetWeak<T>;

        inline explicit Asset(const asset_id_t id) : AssetBase(id) {}

        inline handle_t handle() const noexcept { return handle_t(_id()); }

      protected:
        friend ref_t;
        friend weak_t;
    };

    template<typename T>
    concept asset_type = std::derived_from<T, Asset<T>>;

    template <asset_type T>
    struct AssetHandle<T> {
        asset_id_t id;
    };

    template <asset_type T>
    class AssetReference<T> {
      public:
        // ReSharper disable CppNonExplicitConvertingConstructor

        AssetReference(T &asset) : m_Asset(&asset) { m_Asset->incRef(); }

        AssetReference(T *const asset) : m_Asset(asset) { m_Asset->incRef(); }

        AssetReference(const AssetReference &o) : m_Asset(o.m_Asset) { m_Asset->incRef(); }

        AssetReference(AssetReference &&o) noexcept : m_Asset(o.m_Asset) {} // move constructor doesn't change refcount

        AssetReference &operator=(const AssetReference &rhs) noexcept {
            if (m_Asset && m_Asset != rhs.m_Asset) {
                m_Asset->decRef();
            }

            m_Asset = rhs.m_Asset;
            m_Asset->incRef();
        }

        // ReSharper restore CppNonExplicitConvertingConstructor
      private:
        T *m_Asset;
    };

    template <asset_type T>
    class AssetWeak<T> {
      public:
        // ReSharper disable CppNonExplicitConvertingConstructor


        // ReSharper restore CppNonExplicitConvertingConstructor

      private:
        T *m_Asset;
    };
} // namespace game

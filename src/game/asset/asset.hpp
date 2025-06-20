//
// Created by andy on 6/5/2025.
//

#pragma once

#include <atomic>
#include <concepts>
#include <iostream>
#include <string>

#ifndef NDEBUG
#include <cassert>
#endif

namespace game {
#ifdef ASSETS_64BIT_ID
    /**
     * @brief The type of an asset id
     */
    using asset_id_t = uint64_t;

    /**
     * @brief The type of the asset counter
     */
    using asset_counter_t = std::atomic_uint64_t;
    constexpr asset_id_t STATIC_ID_DOMAIN = 0xFFFF << 48;
#else
    /**
     * @brief The type of an asset id
     */
    using asset_id_t = uint32_t;

    /**
     * @brief The type of the asset counter
     */
    using asset_counter_t = std::atomic_uint32_t;

    constexpr asset_id_t STATIC_ID_DOMAIN = 0xFF << 24;
#endif

    /**
     * \def ASSETS_MAX_REFERENCES
     * \brief The maximum number of references an asset can have at once (determines reference counter size)
     */

#ifndef ASSETS_MAX_REFERENCES
#define ASSETS_MAX_REFERENCES 4294967295U
#endif

#if ASSETS_MAX_REFERENCES <= 255U
    /**
     * @brief The type of an asset reference counter
     */
    using asset_rc_t = std::atomic_uint8_t;
#elif ASSETS_MAX_REFERENCES <= 65535U
    /**
     * @brief The type of an asset reference counter
     */
    using asset_rc_t = std::atomic_uint16_t;
#elif ASSETS_MAX_REFERENCES <= 4294967295U
    /**
     * @brief The type of an asset reference counter
     */
    using asset_rc_t = std::atomic_uint32_t;
#else
    /**
     * @brief The type of an asset reference counter
     */
    using asset_rc_t = std::atomic_uint64_t;
#endif

#ifdef ASSETS_REFCOUNT_BOUNDS_CHECKS
#if ASSETS_MAX_REFERENCES == 18446744073709551615U || ASSETS_MAX_REFERENCES == 4294967295U || ASSETS_MAX_REFERENCES == 65535U || ASSETS_MAX_REFERENCES == 255U
    // max references is equal to the max of an unsigned type, so passing increment bounds **will** overflow
#define ASSET_OVERFLOW_WARNING " (results in integer overflow, expect broken state)"
#else
#define ASSET_OVERFLOW_WARNING
#endif

#define asset_rc_assert_inc(s) assert((s < (ASSETS_MAX_REFERENCES)) && ("Cannot increment reference counter: Too many references to asset" ASSET_OVERFLOW_WARNING))
#define asset_rc_assert_dec(s) assert((s > 0) && ("Cannot decrement reference counter: No existing references to asset (will cause integer underflow, expect broken state)"))
#else
#define asset_rc_assert_inc(s) (s)
#define asset_rc_assert_dec(s) (s)
#endif

    /**
     * @brief The non-atomic integer type for the asset reference counter (used as the return value when actually checking the refcount).
     */
    using asset_refcount_t = asset_rc_t::value_type;

    /**
     * \def asset_rc_assert_inc(s)
     * \param s The increment statement
     * \brief Easy to use assert statement for asset reference counter increments (when not in debug mode, will just evaluate the input parameter)
     */

    /**
     * \def asset_rc_assert_dec(s)
     * \param s The decrement statement
     * \brief Easy to use assert statement for asset reference counter decrements (when not in debug mode, will just evaluate the input parameter)
     */

    /**
     * @brief A strong reference to an asset (counted in the reference count)
     * @tparam T The asset type
     */
    template <typename T>
    class AssetRef;

    /**
     * @brief A weak reference to an asset (not counted in the reference count)
     * @tparam T The asset type
     */
    template <typename T>
    class AssetWeak;

    /**
     * @brief Base class for assets
     *
     * This is used to enable homogeneous storage for assets, which improves memory overhead.
     */
    class AssetBase {
      public:
        /**
         * @brief Base constructor for assets
         * @param id The asset's id
         * @param name The name of the asset
         */
        inline explicit AssetBase(const asset_id_t id, const std::string &name) : m_Id(id), m_Name(name), m_RefCount(0) {}

        inline virtual ~AssetBase() = default;

        /**
         * @brief Get the name of the asset
         * @return The asset's name
         */
        [[nodiscard]] inline std::string name() const noexcept { return m_Name; }

        [[nodiscard]] inline asset_refcount_t refcount() const noexcept { return m_RefCount.load(std::memory_order::relaxed); }

        inline void setKeepAlive() { m_KeepAlive.test_and_set(std::memory_order::seq_cst); }

        inline void unsetKeepAlive() { m_KeepAlive.clear(std::memory_order::seq_cst); }

        [[nodiscard]] inline bool isKeepAlive() const noexcept { return m_KeepAlive.test(std::memory_order::seq_cst); }

      protected:
        /**
         * @brief Get the raw id of the asset
         * @return The raw id of the asset
         */
        [[nodiscard]] inline asset_id_t _id() const noexcept { return m_Id; };

        // we can tell the linter to ignore this looking like a problem for release builds since we also change the definition of asset_rc_assert_* in non-debug builds

        /**
         * @brief Increment the reference counter for this asset
         */
        inline void incRef() noexcept {
            const auto i = m_RefCount.fetch_add(1, std::memory_order::relaxed);
            asset_rc_assert_inc(i);
        } // NOLINT(*-assert-side-effect)

        /**
         * @brief Decrement the reference counter for this asset
         */
        inline void decRef() noexcept {
            const auto i = m_RefCount.fetch_sub(1, std::memory_order::relaxed);
            asset_rc_assert_dec(i);
        } // NOLINT(*-assert-side-effect)

        friend class AssetManager;
        friend class GenericAssetRef;

      private:
        asset_id_t       m_Id;
        asset_rc_t       m_RefCount;
        std::string      m_Name;
        std::atomic_flag m_KeepAlive;
    };

    /**
     * @brief A typed asset handle
     * @tparam T The asset type
     */
    template <typename T>
    struct AssetHandle;

    /**
     * @brief A type-safe base class for assets
     * @tparam T The asset type. This should be set to the actual asset type, unless your asset isn't a concrete type (i.e. maintain the template chain if you are writing an
     * abstract asset type).
     */
    template <typename T>
    class Asset : public AssetBase {
      public:
        /**
         * @brief The type of this asset's handle
         */
        using handle_t = AssetHandle<T>;

        /**
         * @brief The type of a reference to this asset
         */
        using ref_t = AssetRef<T>;

        /**
         * @brief The type of a weak reference to this asset
         */
        using weak_t = AssetWeak<T>;

        /**
         * @brief The constructor for an asset. Mirrors AssetBase's constructor.
         * @param id The id of this asset
         * @param name The name of this asset
         */
        inline explicit Asset(const asset_id_t id, const std::string &name) : AssetBase(id, name) {}

        /**
         * @brief Get a type-safe handle to this asset
         * @return A type-safe handle to this asset
         */
        inline handle_t handle() const noexcept { return handle_t(_id()); }

      protected:
        friend ref_t;
        friend weak_t;
    };

    template <typename T>
    concept asset_type = std::derived_from<T, Asset<T>>;

    template <asset_type T>
    struct AssetHandle<T> {
        asset_id_t id;
    };

    template <asset_type T>
    class AssetRef<T> {
      public:
        // ReSharper disable CppNonExplicitConvertingConstructor

        AssetRef() : m_Asset(nullptr) {};

        /**
         * @brief Constructor accepting a non-const lvalue reference to the asset we are constructing a reference to.
         * @param asset The asset to make a reference to.
         */
        AssetRef(T &asset) : m_Asset(&asset) { m_Asset->incRef(); }

        /**
         * @brief Constructor accepting a non-const pointer to the asset we are constructing a reference to.
         * @param asset  The asset to make a reference to.
         */
        AssetRef(T *const asset) : m_Asset(asset) { m_Asset->incRef(); }

        /**
         * @brief Copy constructor. Will change refcount.
         * @param o
         */
        AssetRef(const AssetRef &o) : m_Asset(o.m_Asset) { m_Asset->incRef(); }

        AssetRef(AssetRef &&other) noexcept : m_Asset(other.m_Asset) { other.m_Asset = nullptr; }

        AssetRef &operator=(AssetRef &&other) noexcept {
            if (this == &other)
                return *this;
            m_Asset       = other.m_Asset;
            other.m_Asset = nullptr;
            return *this;
        }

        /**
         * @brief Copy assignment operator. Will change refcount.
         * @param rhs
         * @return
         */
        AssetRef &operator=(const AssetRef &rhs) noexcept {
            if (m_Asset && m_Asset != rhs.m_Asset) {
                m_Asset->decRef();
            }

            m_Asset = rhs.m_Asset;
            m_Asset->incRef();
            return *this;
        }

        ~AssetRef() {
            if (m_Asset != nullptr) {
                m_Asset->decRef();
            }
        }

        /**
         * @brief Deref operator.
         * @return A reference to the referenced asset.
         */
        T &operator*() const { return *m_Asset; }

        /**
         * @brief Arrow operator.
         * @return A pointer to the referenced asset.
         */
        T *operator->() const noexcept { return m_Asset; }

        /**
         * @brief Boolean conversion. Returns true if this reference is not nullptr.
         */
        operator bool() const noexcept { return m_Asset != nullptr; }

        /**
         * @brief Equality operator (against nullptr)
         * @return True if this doesn't reference an asset.
         */
        bool operator==(std::nullptr_t) const noexcept { return m_Asset == nullptr; }

        /**
         * @brief Equality operator (against pointer to asset)
         * @param rhs
         * @return If this reference refers to the asset pointer to by the pointer.
         */
        bool operator==(const T *rhs) const noexcept { return m_Asset == rhs; }

        /**
         * @brief Standard equality operator
         * @param rhs
         * @return If both asset references refer to the same asset.
         */
        bool operator==(const AssetRef &rhs) const noexcept { return m_Asset == rhs.m_Asset; }

        /**
         * @brief Equality operator against weak references
         * @param rhs
         * @return If this references the same asset as the weak reference does.
         */
        bool operator==(const AssetWeak<T> &rhs) const noexcept;

        /**
         * @brief Make a weak reference from this
         * @return A weak reference to the asset this references
         */
        [[nodiscard]] AssetWeak<T> weak() const { return m_Asset; }

        void reset() {
            if (m_Asset) {
                m_Asset->decRef();
                m_Asset = nullptr;
            }
        }

        // ReSharper restore CppNonExplicitConvertingConstructor
      private:
        T *m_Asset;
    };

    template <asset_type T>
    class AssetWeak<T> {
      public:
        // ReSharper disable CppNonExplicitConvertingConstructor
        /**
         * @brief Constructor accepting a non-const lvalue reference to an asset
         * @param asset The asset
         */
        AssetWeak(T &asset) : m_Asset(&asset) {}

        /**
         * @brief Constructor accepting a strong reference to an asset
         * @param asset The asset reference
         */
        AssetWeak(const AssetRef<T> &asset) : m_Asset(asset.m_Asset) {}

        /**
         * @brief Constructor accepting a non-const pointer to an asset
         * @param asset The asset
         */
        AssetWeak(T *const asset) : m_Asset(asset) {}

        // this class can use default copy and move constructors since it doesn't do any reference counting

        /**
         * @brief Convert this weak reference to a strong reference
         * @return A strong reference to the asset
         * This does no checks to see if our weak reference is valid, so you can still get an access violation using the returned reference. This is only important since using the
         * asset from a reference should require incrementing the refcount.
         *
         * @note Try not to call this frequently, as it does increment an atomic counter. Instead store the reference while you need it and remove when you don't anymore. If you
         * need freeform access and aren't worried about lifetime, obtain and use a raw pointer.
         */
        [[nodiscard]] AssetRef<T> lock() const { return AssetRef<T>(m_Asset); }

        [[nodiscard]] AssetHandle<T> handle() const noexcept { return m_Asset->handle(); }

        [[nodiscard]] std::string name() const noexcept { return m_Asset->name(); }

        [[nodiscard]] asset_refcount_t refcount() const noexcept { return m_Asset->refcount(); }

        // ReSharper restore CppNonExplicitConvertingConstructor

      private:
        T *m_Asset;
        friend class AssetRef<T>;
    };

    template <asset_type T>
    bool AssetRef<T>::operator==(const AssetWeak<T> &rhs) const noexcept {
        return m_Asset == rhs.m_Asset;
    }

    class GenericAssetRef {
        AssetBase *m_Asset;

      public:
        // ReSharper disable once CppNonExplicitConvertingConstructor
        GenericAssetRef(AssetBase *asset) : m_Asset(asset) { m_Asset->incRef(); }

        template <asset_type T>
        AssetRef<T> as() const {
            return AssetRef<T>(dynamic_cast<T *>(m_Asset));
        }

        template<asset_type T>
        operator AssetRef<T>() const {
            return AssetRef<T>(dynamic_cast<T *>(m_Asset));
        }

        void reset() {
            if (m_Asset) {
                m_Asset->decRef();
                m_Asset = nullptr;
            }
        }

        ~GenericAssetRef() {
            if (m_Asset != nullptr) {
                m_Asset->decRef();
            }
        }

        /**
         * @brief Copy constructor. Will change refcount.
         * @param o
         */
        GenericAssetRef(const GenericAssetRef &o) : m_Asset(o.m_Asset) { m_Asset->incRef(); }

        GenericAssetRef(GenericAssetRef &&other) noexcept : m_Asset(other.m_Asset) { other.m_Asset = nullptr; }

        GenericAssetRef &operator=(GenericAssetRef &&other) noexcept {
            if (this == &other)
                return *this;
            m_Asset       = other.m_Asset;
            other.m_Asset = nullptr;
            return *this;
        }

        /**
         * @brief Copy assignment operator. Will change refcount.
         * @param rhs
         * @return
         */
        GenericAssetRef &operator=(const GenericAssetRef &rhs) noexcept {
            if (m_Asset && m_Asset != rhs.m_Asset) {
                m_Asset->decRef();
            }

            m_Asset = rhs.m_Asset;
            m_Asset->incRef();
            return *this;
        }
    };
} // namespace game

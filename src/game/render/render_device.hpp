//
// Created by andy on 6/5/2025.
//

#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <optional>

namespace game {

    class RenderDevice {
      public:
        RenderDevice();
        ~RenderDevice();

        inline const vk::raii::Instance &instance() const noexcept { return m_Instance; };

        inline const vk::raii::PhysicalDevice &physicalDevice() const noexcept { return m_PhysicalDevice; };

        inline const vk::raii::Device &device() const noexcept { return m_Device; };

        inline uint32_t mainFamily() const noexcept { return m_MainFamily; };

        inline uint32_t presentFamily() const noexcept { return m_PresentFamily; };

        inline const std::optional<uint32_t> &exclusiveTransferFamily() const noexcept { return m_ExclusiveTransferFamily; };

        inline const vk::raii::Queue &mainQueue() const noexcept { return m_MainQueue; };

        inline const vk::raii::Queue &presentQueue() const noexcept { return m_PresentQueue; };

        inline const std::optional<vk::raii::Queue> &exclusiveTransferQueue() const noexcept { return m_ExclusiveTransferQueue; };

        template <std::ranges::contiguous_range R>
        void resetFences(R &&fences) const {
            m_Device.resetFences(std::forward<R>(fences));
        }

      private:
        vk::raii::Context        m_Context;
        vk::raii::Instance       m_Instance{nullptr};
        vk::raii::PhysicalDevice m_PhysicalDevice{nullptr};
        vk::raii::Device         m_Device{nullptr};

        uint32_t                m_MainFamily              = UINT32_MAX;
        uint32_t                m_PresentFamily           = UINT32_MAX;
        std::optional<uint32_t> m_ExclusiveTransferFamily = std::nullopt;

        vk::raii::Queue                m_MainQueue{nullptr};
        vk::raii::Queue                m_PresentQueue{nullptr};
        std::optional<vk::raii::Queue> m_ExclusiveTransferQueue = std::nullopt;
    };
} // namespace game

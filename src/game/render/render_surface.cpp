//
// Created by andy on 6/5/2025.
//

#include "render_surface.hpp"

namespace game {
    RenderSurface::RenderSurface(std::shared_ptr<Window> window, std::shared_ptr<RenderDevice> renderDevice)
        : m_Window(std::move(window)), m_RenderDevice(std::move(renderDevice)) {
        VkSurfaceKHR surf;
        glfwCreateWindowSurface(*m_RenderDevice->instance(), m_Window->window(), nullptr, &surf);
        m_Surface = vk::raii::SurfaceKHR(m_RenderDevice->instance(), surf);

        createSwapchain();
    }

    RenderSurface::~RenderSurface() = default;

    inline vk::SurfaceFormatKHR selectSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &formats) {
        auto fallback = formats[0];

        for (const auto &format : formats) {
            if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                return format;
            }
        }

        return fallback;
    }

    inline vk::PresentModeKHR selectPresentMode(const std::vector<vk::PresentModeKHR> &presentModes) {
        for (const auto &mode : presentModes) {
            if (mode == vk::PresentModeKHR::eMailbox) {
                return mode;
            }
        }

        return vk::PresentModeKHR::eFifo;
    }

    void RenderSurface::createSwapchain() {
        auto capabilities   = m_RenderDevice->physicalDevice().getSurfaceCapabilitiesKHR(m_Surface);
        auto presentModes   = m_RenderDevice->physicalDevice().getSurfacePresentModesKHR(m_Surface);
        auto surfaceFormats = m_RenderDevice->physicalDevice().getSurfaceFormatsKHR(m_Surface);

        uint32_t minImageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && minImageCount > capabilities.maxImageCount)
            minImageCount = capabilities.maxImageCount;

        auto sf       = selectSurfaceFormat(surfaceFormats);
        m_ColorSpace  = sf.colorSpace;
        m_Format      = sf.format;
        m_PresentMode = selectPresentMode(presentModes);

        if (capabilities.currentExtent.height == UINT32_MAX) {
            auto windowExtent = m_Window->getExtent();
            m_Extent          = vk::Extent2D{
                std::clamp(windowExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
                std::clamp(windowExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
            };
        } else {
            m_Extent = capabilities.currentExtent;
        }

        vk::SharingMode       sharingMode;
        std::vector<uint32_t> queueFamilies;
        if (m_RenderDevice->mainFamily() == m_RenderDevice->presentFamily()) {
            sharingMode = vk::SharingMode::eExclusive;
        } else {
            sharingMode   = vk::SharingMode::eConcurrent;
            queueFamilies = {m_RenderDevice->mainFamily(), m_RenderDevice->presentFamily()};
        }

        vk::SwapchainCreateInfoKHR createInfo{};
        createInfo.setClipped(true)
            .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
            .setImageArrayLayers(1)
            .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst)
            .setOldSwapchain(m_Swapchain)
            .setPreTransform(capabilities.currentTransform)
            .setSurface(m_Surface)
            .setMinImageCount(minImageCount)
            .setPresentMode(m_PresentMode)
            .setImageColorSpace(m_ColorSpace)
            .setImageFormat(m_Format)
            .setImageExtent(m_Extent)
            .setImageSharingMode(sharingMode)
            .setQueueFamilyIndices(queueFamilies)
            .setPresentMode(m_PresentMode);

        m_Swapchain = vk::raii::SwapchainKHR(m_RenderDevice->device(), createInfo);
        m_Images    = m_Swapchain.getImages();
    }

    std::optional<std::tuple<vk::Image, uint32_t>> RenderSurface::acquireNextImage(vk::Semaphore semaphore) {
        try {
            const auto [res, index] = m_Swapchain.acquireNextImage(UINT64_MAX, semaphore);
            if (res != vk::Result::eSuccess)
                m_NeedsRebuild = true;

            return std::make_tuple(m_Images[index], index);
        } catch (const vk::OutOfDateKHRError &e) {
            m_NeedsRebuild = true;
            return std::nullopt;
        }
    }

    bool RenderSurface::checkRebuildSwapchain() {
        if (m_NeedsRebuild) {
            m_RenderDevice->device().waitIdle(); // wait idle so we don't step on any toes
            createSwapchain();
            m_NeedsRebuild = false;
            return true;
        }
        return false;
    }

    void RenderSurface::present(uint32_t index, vk::Semaphore wait) {
        vk::PresentInfoKHR presentInfo{};
        presentInfo.setSwapchains(*m_Swapchain);
        presentInfo.setImageIndices(index);
        presentInfo.setWaitSemaphores(wait);
        try {
            const auto r   = m_RenderDevice->presentQueue().presentKHR(presentInfo);
            m_NeedsRebuild = r != vk::Result::eSuccess;
        } catch (const vk::OutOfDateKHRError &e) {
            m_NeedsRebuild = true;
        }
    }
} // namespace game

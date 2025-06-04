#include "render.hpp"

#include "vulkan/vulkan_enums.hpp"
#include "window.hpp"

#include <algorithm>

namespace game {
    RenderDevice::RenderDevice() {
        {
            vk::InstanceCreateInfo createInfo{};
            vk::ApplicationInfo    appInfo{};
            appInfo.setApiVersion(vk::ApiVersion13);

            createInfo.setPApplicationInfo(&appInfo);

            uint32_t     count;
            const char **extensions = glfwGetRequiredInstanceExtensions(&count);
            createInfo.setPpEnabledExtensionNames(extensions);
            createInfo.setEnabledExtensionCount(count);
            m_Instance = vk::raii::Instance(m_Context, createInfo);
        }

        m_PhysicalDevice = m_Instance.enumeratePhysicalDevices()[0];

        {
            auto queueFamilyProperties = m_PhysicalDevice.getQueueFamilyProperties();
            for (uint32_t i = 0; i < queueFamilyProperties.size(); i++) {
                const auto &props = queueFamilyProperties[i];
                if (m_MainFamily == UINT32_MAX && props.queueFlags & vk::QueueFlagBits::eGraphics) {
                    m_MainFamily = i;

                    if (glfwGetPhysicalDevicePresentationSupport(*m_Instance, *m_PhysicalDevice, i)) {
                        m_PresentFamily = i;
                    }
                }

                if (m_PresentFamily == UINT32_MAX && glfwGetPhysicalDevicePresentationSupport(*m_Instance, *m_PhysicalDevice, i)) {
                    m_PresentFamily = i;
                }

                if (!m_ExclusiveTransferFamily.has_value() && (props.queueFlags & vk::QueueFlagBits::eGraphics) != vk::QueueFlagBits::eGraphics &&
                    (props.queueFlags & vk::QueueFlagBits::eTransfer)) {
                    m_ExclusiveTransferFamily = i;
                }
            }
        }

        {
            std::vector<const char *> extensions = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
            };

            vk::PhysicalDeviceFeatures2 features2{};
            auto                       &features = features2.features;
            features.geometryShader              = true;
            features.tessellationShader          = true;
            features.drawIndirectFirstInstance   = true;
            features.multiDrawIndirect           = true;
            features.wideLines                   = true;
            features.largePoints                 = true;

            vk::PhysicalDeviceVulkan11Features vulkan11Features{};

            vk::PhysicalDeviceVulkan12Features vulkan12Features{};
            vulkan12Features.bufferDeviceAddress = true;
            vulkan12Features.descriptorIndexing  = true;
            vulkan12Features.timelineSemaphore   = true;
            vulkan12Features.drawIndirectCount   = true;

            vk::PhysicalDeviceVulkan13Features vulkan13Features{};
            vulkan13Features.dynamicRendering   = true;
            vulkan13Features.inlineUniformBlock = true;
            vulkan13Features.synchronization2   = true;

            vk::PhysicalDeviceShaderObjectFeaturesEXT shaderObjectFeatures{};
            shaderObjectFeatures.shaderObject = true;


            features2.pNext        = &vulkan11Features;
            vulkan11Features.pNext = &vulkan12Features;
            vulkan12Features.pNext = &vulkan13Features;
            vulkan13Features.pNext = &shaderObjectFeatures;


            std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{};
            bool                                   presentQueueSameAsMain = m_MainFamily == m_PresentFamily;
            std::array<float, 1>                   queuePriorities        = {1.0f};

            queueCreateInfos.push_back(vk::DeviceQueueCreateInfo({}, m_MainFamily, queuePriorities));
            if (!presentQueueSameAsMain)
                queueCreateInfos.push_back(vk::DeviceQueueCreateInfo({}, m_PresentFamily, queuePriorities));
            if (m_ExclusiveTransferFamily.has_value())
                queueCreateInfos.push_back(vk::DeviceQueueCreateInfo({}, m_ExclusiveTransferFamily.value(), queuePriorities));

            m_Device = vk::raii::Device(m_PhysicalDevice, vk::DeviceCreateInfo({}, queueCreateInfos, {}, extensions, nullptr, &features2));
        }

        m_MainQueue    = m_Device.getQueue(m_MainFamily, 0);
        m_PresentQueue = m_Device.getQueue(m_PresentFamily, 0);
        if (m_ExclusiveTransferFamily.has_value())
            m_ExclusiveTransferQueue = m_Device.getQueue(m_ExclusiveTransferFamily.value(), 0);
    }

    RenderDevice::~RenderDevice() = default;

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

    SingleTimeCommands::SingleTimeCommands(std::shared_ptr<RenderDevice> renderDevice, uint32_t family, const vk::raii::Queue &queue)
        : m_RenderDevice(std::move(renderDevice)), m_CommandPool(m_RenderDevice->device(), vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, family)), m_Family(family), m_Queue(queue) {

        m_CommandBuffers.reserve(INITIAL_CAPACITY);
        auto commandBuffers = vk::raii::CommandBuffers(m_RenderDevice->device(), vk::CommandBufferAllocateInfo(m_CommandPool, vk::CommandBufferLevel::ePrimary, INITIAL_CAPACITY));
        std::size_t i = 0;
        for (auto&& cmd : commandBuffers) {
            m_CommandBuffers.emplace_back(std::move(cmd), vk::raii::Fence(m_RenderDevice->device(), vk::FenceCreateInfo()));
            m_Available.emplace_back(i++);
        }
    }

    void SingleTimeCommands::pollInUse() {
    }

    void SingleTimeCommands::runCommands(const std::function<void(const vk::raii::CommandBuffer &cmd)>, std::optional<vk::SemaphoreSubmitInfo> waitSemaphore,
                                         std::optional<vk::SemaphoreSubmitInfo> signalSemaphore) {
        auto commandBufferIndex = acquireCommandBuffer();
        // todo
        vk::SubmitInfo2 submitInfo{};
    }

    std::size_t SingleTimeCommands::acquireCommandBuffer() {
        if (m_Available.empty()) expandCapacity();
        std::size_t i = m_Available.front();
        m_Available.pop_front();
        return i;
    }

    void SingleTimeCommands::expandCapacity() {
        // double capacity

        auto commandBuffers = vk::raii::CommandBuffers(m_RenderDevice->device(), vk::CommandBufferAllocateInfo(m_CommandPool, vk::CommandBufferLevel::ePrimary, m_CommandBuffers.size()));
        std::size_t i = m_CommandBuffers.size();
        m_CommandBuffers.reserve(m_CommandBuffers.size() * 2);
        for (auto&& cmd : commandBuffers) {
            m_CommandBuffers.emplace_back(std::move(cmd), vk::raii::Fence(m_RenderDevice->device(), vk::FenceCreateInfo()));
            m_Available.emplace_back(i++);
        }
    }


} // namespace game

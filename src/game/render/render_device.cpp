//
// Created by andy on 6/5/2025.
//

#include "render_device.hpp"
#include <GLFW/glfw3.h>

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
            vulkan13Features.maintenance4       = true;


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
} // namespace game

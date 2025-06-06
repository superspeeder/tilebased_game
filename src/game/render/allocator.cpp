//
// Created by andy on 6/5/2025.
//
#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "allocator.hpp"

namespace game {
    RawBuffer::~RawBuffer() {
        vmaDestroyBuffer(allocator, buffer, allocation);
    }

    RawImage::~RawImage() {
        vmaDestroyImage(allocator, image, allocation);
    }

    Allocator::Allocator(std::shared_ptr<RenderDevice> renderDevice) : m_RenderDevice(std::move(renderDevice)) {
        VmaVulkanFunctions vf{};
        vf.vkGetInstanceProcAddr = m_RenderDevice->instance().getDispatcher()->vkGetInstanceProcAddr;
        vf.vkGetDeviceProcAddr   = m_RenderDevice->device().getDispatcher()->vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.instance               = *m_RenderDevice->instance();
        allocatorCreateInfo.physicalDevice         = *m_RenderDevice->physicalDevice();
        allocatorCreateInfo.device                 = *m_RenderDevice->device();
        allocatorCreateInfo.vulkanApiVersion       = vk::ApiVersion13;
        allocatorCreateInfo.flags                  = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT | VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE4_BIT;
        allocatorCreateInfo.pVulkanFunctions       = &vf;

        if (const VkResult res = vmaCreateAllocator(&allocatorCreateInfo, &m_Allocator); res != VK_SUCCESS) {
            vk::detail::throwResultException(static_cast<vk::Result>(res), "vmaCreateAllocator");
        }
    }

    Allocator::~Allocator() {
        vmaDestroyAllocator(m_Allocator);
    }

    std::shared_ptr<RawBuffer> Allocator::createBufferRaw(const vk::BufferCreateInfo &createInfo, const MemoryUsage memoryUsage) const {
        VmaAllocationCreateInfo allocationCreateInfo = {};
        switch (memoryUsage) {
        case MemoryUsage::Unknown:
            throw std::invalid_argument("Unknown memory usage is not yet implemented");
            break;
        case MemoryUsage::Auto:
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
            break;
        case MemoryUsage::AutoPreferDevice:
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            break;
        case MemoryUsage::AutoPreferHost:
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
            allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
            break;
        }

        VmaAllocationInfo        allocationInfo{};
        VmaAllocation            allocation;
        VkBuffer                 buffer{};
        const VkBufferCreateInfo bufferCreateInfo = createInfo;

        if (const VkResult res = vmaCreateBuffer(m_Allocator, &bufferCreateInfo, &allocationCreateInfo, &buffer, &allocation, &allocationInfo); res != VK_SUCCESS) {
            vk::detail::throwResultException(static_cast<vk::Result>(res), "vmaCreateBuffer");
        }

        return std::make_shared<RawBuffer>(m_Allocator, allocation, allocationInfo, buffer);
    }

    std::shared_ptr<RawImage> Allocator::createImageRaw(const vk::ImageCreateInfo &createInfo, const MemoryUsage memoryUsage) const {
        VmaAllocationCreateInfo allocationCreateInfo = {};
        switch (memoryUsage) {
        case MemoryUsage::Unknown:
            throw std::invalid_argument("Unknown memory usage is not yet implemented");
            break;
        case MemoryUsage::Auto:
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
            break;
        case MemoryUsage::AutoPreferDevice:
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            break;
        case MemoryUsage::AutoPreferHost:
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
            allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
            break;
        }

        VmaAllocationInfo       allocationInfo{};
        VmaAllocation           allocation;
        VkImage                 image{};
        const VkImageCreateInfo imageCreateInfo = createInfo;

        if (const VkResult res = vmaCreateImage(m_Allocator, &imageCreateInfo, &allocationCreateInfo, &image, &allocation, &allocationInfo); res != VK_SUCCESS) {
            vk::detail::throwResultException(static_cast<vk::Result>(res), "vmaCreateImage");
        }

        return std::make_shared<RawImage>(m_Allocator, allocation, allocationInfo, image);
    }

    std::unique_ptr<RawBuffer> Allocator::createBufferRawUnique(const vk::BufferCreateInfo &createInfo, const MemoryUsage memoryUsage) const {
        VmaAllocationCreateInfo allocationCreateInfo = {};
        switch (memoryUsage) {
        case MemoryUsage::Unknown:
            throw std::invalid_argument("Unknown memory usage is not yet implemented");
            break;
        case MemoryUsage::Auto:
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
            break;
        case MemoryUsage::AutoPreferDevice:
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            break;
        case MemoryUsage::AutoPreferHost:
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
            allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
            break;
        }

        VmaAllocationInfo        allocationInfo{};
        VmaAllocation            allocation;
        VkBuffer                 buffer{};
        const VkBufferCreateInfo bufferCreateInfo = createInfo;

        if (const VkResult res = vmaCreateBuffer(m_Allocator, &bufferCreateInfo, &allocationCreateInfo, &buffer, &allocation, &allocationInfo); res != VK_SUCCESS) {
            vk::detail::throwResultException(static_cast<vk::Result>(res), "vmaCreateBuffer");
        }

        return std::make_unique<RawBuffer>(m_Allocator, allocation, allocationInfo, buffer);
    }

    std::unique_ptr<RawImage> Allocator::createImageRawUnique(const vk::ImageCreateInfo &createInfo, const MemoryUsage memoryUsage) const {
        VmaAllocationCreateInfo allocationCreateInfo = {};
        switch (memoryUsage) {
        case MemoryUsage::Unknown:
            throw std::invalid_argument("Unknown memory usage is not yet implemented");
            break;
        case MemoryUsage::Auto:
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
            break;
        case MemoryUsage::AutoPreferDevice:
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            break;
        case MemoryUsage::AutoPreferHost:
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
            allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
            break;
        }

        VmaAllocationInfo       allocationInfo{};
        VmaAllocation           allocation;
        VkImage                 image{};
        const VkImageCreateInfo imageCreateInfo = createInfo;

        if (const VkResult res = vmaCreateImage(m_Allocator, &imageCreateInfo, &allocationCreateInfo, &image, &allocation, &allocationInfo); res != VK_SUCCESS) {
            vk::detail::throwResultException(static_cast<vk::Result>(res), "vmaCreateImage");
        }

        return std::make_unique<RawImage>(m_Allocator, allocation, allocationInfo, image);
    }
} // namespace game

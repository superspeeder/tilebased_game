//
// Created by andy on 6/5/2025.
//

#pragma once
#include "game/render/render_device.hpp"

#include <vk_mem_alloc.h>

namespace game {

    enum class MemoryUsage {
        Unknown          = VMA_MEMORY_USAGE_UNKNOWN,
        Auto             = VMA_MEMORY_USAGE_AUTO,
        AutoPreferDevice = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        AutoPreferHost   = VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
    };

    struct RawBuffer {
        VmaAllocator allocator;
        VmaAllocation allocation;
        VmaAllocationInfo allocationInfo;
        vk::Buffer    buffer;

        ~RawBuffer();
    };

    struct RawImage {
        VmaAllocator allocator;
        VmaAllocation allocation;
        VmaAllocationInfo allocationInfo;
        vk::Image image;

        ~RawImage();
    };

    class Allocator {
      public:
        explicit Allocator(std::shared_ptr<RenderDevice> renderDevice);
        ~Allocator();

        std::shared_ptr<RawBuffer> createBufferRaw(const vk::BufferCreateInfo &createInfo, MemoryUsage memoryUsage) const;
        std::shared_ptr<RawImage>  createImageRaw(const vk::ImageCreateInfo &createInfo, MemoryUsage memoryUsage) const;
        std::unique_ptr<RawBuffer> createBufferRawUnique(const vk::BufferCreateInfo &createInfo, MemoryUsage memoryUsage) const;
        std::unique_ptr<RawImage>  createImageRawUnique(const vk::ImageCreateInfo &createInfo, MemoryUsage memoryUsage) const;

      private:
        std::shared_ptr<RenderDevice> m_RenderDevice;
        VmaAllocator                  m_Allocator;
    };

} // namespace game

//
// Created by andy on 6/5/2025.
//

#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>

namespace game {
    struct ImageProperties {
        vk::Format   format;
        vk::Extent2D extent;
    };
}
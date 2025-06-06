cmake_minimum_required(VERSION 3.30)
project(tilegame)

# Options
option(ASSETS_REFCOUNT_BOUNDS_CHECKS "Enable bounds checking asserts on asset reference counters" OFF)


# Build
set(CMAKE_CXX_STANDARD 23)

include(FetchContent)
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.1)
FetchContent_Declare(spdlog GIT_REPOSITORY https://github.com/gabime/spdlog.git GIT_TAG v1.15.3)
FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)
FetchContent_Declare(VulkanHeaders GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git GIT_TAG v1.4.316)
FetchContent_Declare(vma GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git GIT_TAG v3.3.0)

set(SPDLOG_USE_STD_FORMAT ON)

FetchContent_MakeAvailable(glm spdlog glfw VulkanHeaders vma)

add_executable(tilegame src/game/game.cpp src/game/window.cpp
        src/game/utils.hpp
        src/game/render/frame_manager.cpp
        src/game/render/frame_manager.hpp
        src/game/render/single_time_commands.cpp
        src/game/render/single_time_commands.hpp
        src/game/render/render_system.cpp
        src/game/render/render_system.hpp
        src/game/render/render_device.cpp
        src/game/render/render_device.hpp
        src/game/render/render_surface.cpp
        src/game/render/render_surface.hpp
        src/game/render/render.hpp
        src/game/render/allocator.cpp
        src/game/render/allocator.hpp
        src/game/render/shader_object.cpp
        src/game/render/shader_object.hpp
        src/game/asset/asset.cpp
        src/game/asset/asset.hpp
        src/game/asset/asset_loader.hpp
)
target_include_directories(tilegame PRIVATE src/)
target_link_libraries(tilegame PRIVATE glm::glm spdlog::spdlog glfw Vulkan::Headers GPUOpen::VulkanMemoryAllocator)
target_compile_definitions(tilegame PRIVATE GLM_ENABLE_EXPERIMENTAL GLFW_INCLUDE_NONE GLFW_INCLUDE_VULKAN)

if (ASSETS_REFCOUNT_BOUNDS_CHECKS)
    target_compile_definitions(tilegame PRIVATE ASSETS_REFCOUNT_BOUNDS_CHECKS)
endif()

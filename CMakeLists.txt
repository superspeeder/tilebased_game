cmake_minimum_required(VERSION 3.30)
project(tilegame)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.1)
FetchContent_Declare(spdlog GIT_REPOSITORY https://github.com/gabime/spdlog.git GIT_TAG v1.15.3)
FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)
FetchContent_Declare(VulkanHeaders GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git GIT_TAG v1.4.316)

set(SPDLOG_USE_STD_FORMAT ON)

FetchContent_MakeAvailable(glm spdlog glfw VulkanHeaders)

add_executable(tilegame src/game/game.cpp src/game/window.cpp src/game/render.cpp
        src/game/utils.hpp)
target_include_directories(tilegame PRIVATE src/)
target_link_libraries(tilegame PRIVATE glm::glm spdlog::spdlog glfw Vulkan::Headers) 
target_compile_definitions(tilegame PRIVATE GLM_ENABLE_EXPERIMENTAL GLFW_INCLUDE_NONE GLFW_INCLUDE_VULKAN)


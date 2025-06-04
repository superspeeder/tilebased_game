#include "game/game.hpp"
#include <iostream>

namespace game {
    libload::libload() {
        glfwInit();
    }

    libload::~libload() {
        glfwTerminate();
    }

    Game::Game() {
        m_Window = std::make_shared<Window>();
        m_RenderDevice = std::make_shared<RenderDevice>();
        m_RenderSurface = std::make_shared<RenderSurface>(m_Window, m_RenderDevice);

        std::cout << "Swapchain has " << m_RenderSurface->images().size() << " images" << std::endl;
    }

    Game::~Game() = default;

    void Game::run() {
        while (!m_Window->shouldClose()) {
            glfwPollEvents();
        }
    }
}

int main() {
    auto game = std::make_unique<game::Game>();
    game->run();
    return 0;
}

#pragma once

#include <GLFW/glfw3.h>
#include <memory>

#include "game/window.hpp"
#include "game/render.hpp"

namespace game {
    struct libload {
        libload();
        ~libload();
    };

    class Game {
    public:
        Game();
        ~Game();

        void run();

    private:
        libload _libload{};
        std::shared_ptr<Window> m_Window;        
        std::shared_ptr<RenderDevice> m_RenderDevice;
        std::shared_ptr<RenderSurface> m_RenderSurface;
    };
}

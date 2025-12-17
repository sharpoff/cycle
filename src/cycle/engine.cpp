#include "cycle/engine.h"

#include "cycle/game/game.h"
#include "cycle/input.h"
#include "cycle/logger.h"
#include "cycle/physics/physics.h"
#include "cycle/renderer.h"

#include <chrono>

#include <SDL3/SDL.h>

namespace Engine
{
    void Init(const char *title, uint32_t width, uint32_t height)
    {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            LOGE("Failed to initialize SDL: %s", SDL_GetError());
            exit(EXIT_FAILURE);
        }

        window = SDL_CreateWindow(title, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
        if (!window) {
            LOGE("Failed to create SDL window: %s", SDL_GetError());
            exit(EXIT_FAILURE);
        }

        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

        Renderer::Init(window);
        Input::Init();
        Physics::Init();
        Game::Init();

        running = true;
    }

    void Shutdown()
    {
        Game::Shutdown();
        Physics::Shutdown();
        Input::Shutdown();
        Renderer::Shutdown();
    }

    void Update()
    {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto        endTime = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration<float, std::milli>(endTime - startTime).count() / 1000.0;
        startTime = endTime;
    }

    bool IsRunning()
    {
        return running;
    }

    SDL_Window *GetWindow()
    {
        return window;
    }
} // namespace Engine
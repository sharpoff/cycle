#pragma once

#include "SDL3/SDL_video.h"

namespace Engine
{
    void Init(const char *title, uint32_t width, uint32_t height);
    void Shutdown();

    void Update();

    bool IsRunning();
    SDL_Window *GetWindow();

    inline double deltaTime = 0.0f;
    inline bool running = false;

    inline SDL_Window *window;
}
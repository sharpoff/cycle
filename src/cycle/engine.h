#pragma once

#include "SDL3/SDL_video.h"

#include "cycle/game.h"
#include "cycle/input/input.h"
#include "cycle/physics.h"
#include "cycle/renderer.h"

class Engine
{
public:
    Engine() = default;
    ~Engine() = default;

    void init(const char *title, uint32_t width, uint32_t height);
    void shutdown();

    void run();

    bool        isRunning();
    SDL_Window *getWindow();

private:
    void processEvents();

    double deltaTime = 0.0f;
    bool   minimized = false;
    bool running = false;

    Renderer renderer;
    Camera   camera;
    Physics  physics;
    Input    input;
    Game     game;

    SDL_Window *window;
};
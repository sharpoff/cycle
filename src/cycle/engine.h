#pragma once

#include "SDL3/SDL_video.h"

#include "cycle/editor.h"
#include "cycle/input/input.h"
#include "cycle/renderer.h"
#include "cycle/world.h"

class Engine
{
public:
    void init(const char *title, uint32_t width, uint32_t height);
    void shutdown();

    void run();

    bool isRunning() { return running; }

    World &getWorld() { return world; }

private:
    void processEvents();

    double deltaTime = 0.0f;
    bool   minimized = false;
    bool   running = false;

    Renderer renderer;
    Camera   camera;
    Input    input;
    World    world;
    Editor   editor;

    SDL_Window *window;
};